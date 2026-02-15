// src/adapters/platform/win32/storage/MFTScanner.cpp
#include "MFTScanner.h"
#include <adapters/platform/win32/core/Win32ErrorHandler.h>
#include <adapters/platform/win32/core/Win32HandleFactory.h>
#include <chrono>
#include <algorithm>
#include <cwctype>
#include <cstring>

namespace winsetup::adapters::platform {

    namespace {
        constexpr size_t BUFFER_SIZE = 64 * 1024;

        std::wstring ToLower(std::wstring str) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return str;
        }

        bool EndsWith(const std::wstring& str, const std::wstring& suffix) {
            if (suffix.size() > str.size()) return false;
            return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                [](wchar_t a, wchar_t b) {
                    return std::towlower(a) == std::towlower(b);
                });
        }

        std::wstring ExtractFileName(const std::wstring& path) {
            size_t pos = path.find_last_of(L"\\/");
            return pos != std::wstring::npos ? path.substr(pos + 1) : path;
        }

        std::wstring GetParentPath(const std::wstring& path) {
            size_t pos = path.find_last_of(L"\\/");
            return pos != std::wstring::npos ? path.substr(0, pos) : L"";
        }

        FILETIME LargeIntegerToFileTime(const LARGE_INTEGER& li) noexcept {
            FILETIME ft;
            ft.dwLowDateTime = static_cast<DWORD>(li.QuadPart & 0xFFFFFFFF);
            ft.dwHighDateTime = static_cast<DWORD>(li.QuadPart >> 32);
            return ft;
        }
    }

    std::wstring MFTScanner::NormalizeVolumePath(const std::wstring& volumePath) {
        std::wstring normalized = volumePath;

        if (normalized.length() == 2 && normalized[1] == L':') {
            normalized = L"\\\\.\\" + normalized;
        }
        else if (normalized.length() == 3 && normalized[1] == L':' && normalized[2] == L'\\') {
            normalized = L"\\\\.\\" + normalized.substr(0, 2);
        }
        else if (normalized.find(L"\\\\?\\") == 0) {
            if (normalized.back() == L'\\') {
                normalized.pop_back();
            }
        }

        return normalized;
    }

    std::wstring MFTScanner::NormalizeFilePath(const std::wstring& filePath) {
        std::wstring normalized = filePath;

        if (!normalized.empty() && (normalized[0] == L'\\' || normalized[0] == L'/')) {
            normalized = normalized.substr(1);
        }

        std::replace(normalized.begin(), normalized.end(), L'/', L'\\');

        while (normalized.find(L"\\\\") != std::wstring::npos) {
            size_t pos = normalized.find(L"\\\\");
            normalized.erase(pos, 1);
        }

        return ToLower(normalized);
    }

    adapters::platform::UniqueHandle MFTScanner::OpenVolumeHandle(const std::wstring& volumePath) {
        std::wstring normalizedPath = NormalizeVolumePath(volumePath);

        HANDLE hVolume = CreateFileW(
            normalizedPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hVolume == INVALID_HANDLE_VALUE) {
            return adapters::platform::UniqueHandle();
        }

        return Win32HandleFactory::MakeHandle(hVolume);
    }

    domain::Expected<USN_JOURNAL_DATA> MFTScanner::QueryUSNJournal(HANDLE hVolume) {
        USN_JOURNAL_DATA journalData{};
        DWORD bytesReturned = 0;

        BOOL result = DeviceIoControl(
            hVolume,
            FSCTL_QUERY_USN_JOURNAL,
            nullptr,
            0,
            &journalData,
            sizeof(journalData),
            &bytesReturned,
            nullptr
        );

        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_JOURNAL_NOT_ACTIVE) {
                return domain::Error{
                    L"USN Journal is not active on this volume",
                    error,
                    domain::ErrorCategory::Volume
                };
            }
            return domain::Error{
                L"Failed to query USN journal",
                error,
                domain::ErrorCategory::Volume
            };
        }

        return journalData;
    }

    bool MFTScanner::ParseUSNRecord(const USN_RECORD* record, MFTFileRecord& outFileRecord) {
        if (!record) return false;

        outFileRecord.fileReferenceNumber = record->FileReferenceNumber;
        outFileRecord.parentFileReferenceNumber = record->ParentFileReferenceNumber;
        outFileRecord.fileSize = 0;
        outFileRecord.fileAttributes = record->FileAttributes;

        outFileRecord.creationTime = LargeIntegerToFileTime(record->TimeStamp);
        outFileRecord.lastAccessTime = LargeIntegerToFileTime(record->TimeStamp);
        outFileRecord.lastWriteTime = LargeIntegerToFileTime(record->TimeStamp);

        outFileRecord.isDirectory = (record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (record->FileNameLength > 0 && record->FileNameLength < 32768) {
            size_t charCount = record->FileNameLength / sizeof(WCHAR);
            outFileRecord.fileName.assign(record->FileName, charCount);
        }

        return !outFileRecord.fileName.empty();
    }

    domain::Expected<void> MFTScanner::ReadUSNJournal(
        HANDLE hVolume,
        const USN_JOURNAL_DATA& journalData,
        std::vector<MFTFileRecord>& outRecords
    ) {
        std::vector<BYTE> buffer(BUFFER_SIZE);

        MFT_ENUM_DATA med{};
        med.StartFileReferenceNumber = 0;
        med.LowUsn = 0;
        med.HighUsn = journalData.NextUsn;

        DWORD bytesReturned = 0;
        uint32_t filesScanned = 0;

        while (true) {
            BOOL result = DeviceIoControl(
                hVolume,
                FSCTL_ENUM_USN_DATA,
                &med,
                sizeof(med),
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesReturned,
                nullptr
            );

            if (!result) {
                DWORD error = GetLastError();
                if (error == ERROR_HANDLE_EOF) {
                    break;
                }
                return domain::Error{
                    L"Failed to enumerate USN data",
                    error,
                    domain::ErrorCategory::Volume
                };
            }

            if (bytesReturned < sizeof(USN)) {
                break;
            }

            DWORD dwRetBytes = bytesReturned - sizeof(USN);
            USN* pUsn = reinterpret_cast<USN*>(buffer.data());
            med.StartFileReferenceNumber = *pUsn;

            BYTE* pRecord = buffer.data() + sizeof(USN);

            while (dwRetBytes > 0) {
                USN_RECORD* record = reinterpret_cast<USN_RECORD*>(pRecord);

                if (record->RecordLength == 0 || record->RecordLength > dwRetBytes) {
                    break;
                }

                MFTFileRecord fileRecord;
                if (ParseUSNRecord(record, fileRecord)) {
                    outRecords.push_back(std::move(fileRecord));
                    filesScanned++;

                    if (filesScanned >= mMaxFilesToScan) {
                        return domain::Expected<void>();
                    }
                }

                pRecord += record->RecordLength;
                dwRetBytes -= record->RecordLength;
            }
        }

        return domain::Expected<void>();
    }

    void MFTScanner::BuildFilePathMap(const std::vector<MFTFileRecord>& records) {
        mFileRecordMap.clear();
        mPathToRefNumberMap.clear();

        for (const auto& record : records) {
            mFileRecordMap[record.fileReferenceNumber] = record;
        }

        for (const auto& record : records) {
            std::wstring fullPath = GetFullPath(record.fileReferenceNumber);
            if (!fullPath.empty()) {
                mPathToRefNumberMap[ToLower(fullPath)] = record.fileReferenceNumber;
            }
        }
    }

    std::wstring MFTScanner::GetFullPath(uint64_t fileRefNumber) {
        std::wstring path;
        uint64_t currentRef = fileRefNumber;
        int depth = 0;
        const int maxDepth = 256;

        while (currentRef != 0 && depth < maxDepth) {
            auto it = mFileRecordMap.find(currentRef);
            if (it == mFileRecordMap.end()) {
                break;
            }

            const auto& record = it->second;

            if (record.fileName == L"." || record.fileName == L"..") {
                break;
            }

            if (!path.empty()) {
                path = record.fileName + L"\\" + path;
            }
            else {
                path = record.fileName;
            }

            if (currentRef == record.parentFileReferenceNumber) {
                break;
            }

            currentRef = record.parentFileReferenceNumber;
            depth++;
        }

        return path;
    }

    domain::Expected<MFTScanResult> MFTScanner::ScanVolume(const std::wstring& volumePath) {
        auto startTime = std::chrono::high_resolution_clock::now();

        auto hVolume = OpenVolumeHandle(volumePath);
        if (!hVolume) {
            return domain::Error{
                L"Failed to open volume: " + volumePath,
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        auto journalResult = QueryUSNJournal(Win32HandleFactory::ToWin32Handle(hVolume));
        if (!journalResult.HasValue()) {
            return journalResult.GetError();
        }

        MFTScanResult result{};

        auto readResult = ReadUSNJournal(
            Win32HandleFactory::ToWin32Handle(hVolume),
            journalResult.Value(),
            result.files
        );

        if (!readResult.HasValue()) {
            return readResult.GetError();
        }

        BuildFilePathMap(result.files);

        result.totalFiles = 0;
        result.totalDirectories = 0;
        result.totalSize = 0;

        for (const auto& file : result.files) {
            if (file.isDirectory) {
                result.totalDirectories++;
            }
            else {
                result.totalFiles++;
                result.totalSize += file.fileSize;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.scanDurationMs = duration.count() / 1000.0;

        return result;
    }

    domain::Expected<bool> MFTScanner::FileExists(
        const std::wstring& volumePath,
        const std::wstring& filePath
    ) {
        if (mPathToRefNumberMap.empty()) {
            auto scanResult = ScanVolume(volumePath);
            if (!scanResult.HasValue()) {
                return scanResult.GetError();
            }
        }

        std::wstring normalizedPath = NormalizeFilePath(filePath);
        return mPathToRefNumberMap.find(normalizedPath) != mPathToRefNumberMap.end();
    }

    domain::Expected<MFTFileRecord> MFTScanner::FindFile(
        const std::wstring& volumePath,
        const std::wstring& filePath
    ) {
        if (mPathToRefNumberMap.empty()) {
            auto scanResult = ScanVolume(volumePath);
            if (!scanResult.HasValue()) {
                return scanResult.GetError();
            }
        }

        std::wstring normalizedPath = NormalizeFilePath(filePath);
        auto it = mPathToRefNumberMap.find(normalizedPath);

        if (it == mPathToRefNumberMap.end()) {
            return domain::Error{
                L"File not found: " + filePath,
                ERROR_FILE_NOT_FOUND,
                domain::ErrorCategory::Volume
            };
        }

        auto recordIt = mFileRecordMap.find(it->second);
        if (recordIt == mFileRecordMap.end()) {
            return domain::Error{
                L"File record not found",
                ERROR_FILE_NOT_FOUND,
                domain::ErrorCategory::Volume
            };
        }

        return recordIt->second;
    }

    domain::Expected<std::vector<MFTFileRecord>> MFTScanner::FindFilesByExtension(
        const std::wstring& volumePath,
        const std::wstring& extension
    ) {
        if (mFileRecordMap.empty()) {
            auto scanResult = ScanVolume(volumePath);
            if (!scanResult.HasValue()) {
                return scanResult.GetError();
            }
        }

        std::vector<MFTFileRecord> matchingFiles;
        std::wstring lowerExt = ToLower(extension);

        if (!lowerExt.empty() && lowerExt[0] != L'.') {
            lowerExt = L"." + lowerExt;
        }

        for (const auto& [refNumber, record] : mFileRecordMap) {
            if (!record.isDirectory && EndsWith(ToLower(record.fileName), lowerExt)) {
                matchingFiles.push_back(record);
            }
        }

        return matchingFiles;
    }

    domain::Expected<uint64_t> MFTScanner::CalculateDirectorySize(
        const std::wstring& volumePath,
        const std::wstring& directoryPath
    ) {
        if (mPathToRefNumberMap.empty()) {
            auto scanResult = ScanVolume(volumePath);
            if (!scanResult.HasValue()) {
                return scanResult.GetError();
            }
        }

        std::wstring normalizedDir = NormalizeFilePath(directoryPath);
        uint64_t totalSize = 0;

        for (const auto& [path, refNumber] : mPathToRefNumberMap) {
            if (path.find(normalizedDir) == 0) {
                auto recordIt = mFileRecordMap.find(refNumber);
                if (recordIt != mFileRecordMap.end() && !recordIt->second.isDirectory) {
                    totalSize += recordIt->second.fileSize;
                }
            }
        }

        return totalSize;
    }

}
