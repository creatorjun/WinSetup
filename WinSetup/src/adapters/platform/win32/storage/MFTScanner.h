// src\adapters\platform\win32\storage\MFTScanner.h
// src/adapters/platform/win32/storage/MFTScanner.h
#pragma once

#include <domain/primitives/Expected.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <Windows.h>
#include <winioctl.h>

namespace winsetup::adapters::platform {

    struct MFTFileRecord {
        uint64_t fileReferenceNumber;
        uint64_t parentFileReferenceNumber;
        std::wstring fileName;
        uint64_t fileSize;
        uint32_t fileAttributes;
        FILETIME creationTime;
        FILETIME lastAccessTime;
        FILETIME lastWriteTime;
        bool isDirectory;

        [[nodiscard]] bool IsSystemFile() const noexcept {
            return (fileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
        }

        [[nodiscard]] bool IsHidden() const noexcept {
            return (fileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
        }

        [[nodiscard]] bool IsReadOnly() const noexcept {
            return (fileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
        }
    };

    struct MFTScanResult {
        std::vector<MFTFileRecord> files;
        uint64_t totalFiles;
        uint64_t totalDirectories;
        uint64_t totalSize;
        double scanDurationMs;

        [[nodiscard]] size_t GetFileCount() const noexcept {
            return files.size();
        }

        [[nodiscard]] double GetAverageScanSpeed() const noexcept {
            return scanDurationMs > 0.0 ? (static_cast<double>(totalFiles) / scanDurationMs * 1000.0) : 0.0;
        }
    };

    class MFTScanner {
    public:
        MFTScanner() = default;
        ~MFTScanner() = default;

        MFTScanner(const MFTScanner&) = delete;
        MFTScanner& operator=(const MFTScanner&) = delete;

        [[nodiscard]] domain::Expected<MFTScanResult> ScanVolume(
            const std::wstring& volumePath
        );

        [[nodiscard]] domain::Expected<bool> FileExists(
            const std::wstring& volumePath,
            const std::wstring& filePath
        );

        [[nodiscard]] domain::Expected<MFTFileRecord> FindFile(
            const std::wstring& volumePath,
            const std::wstring& filePath
        );

        [[nodiscard]] domain::Expected<std::vector<MFTFileRecord>> FindFilesByExtension(
            const std::wstring& volumePath,
            const std::wstring& extension
        );

        [[nodiscard]] domain::Expected<uint64_t> CalculateDirectorySize(
            const std::wstring& volumePath,
            const std::wstring& directoryPath
        );

        void SetMaxFilesToScan(uint32_t maxFiles) noexcept {
            mMaxFilesToScan = maxFiles;
        }

        void SetScanTimeout(uint32_t timeoutMs) noexcept {
            mScanTimeoutMs = timeoutMs;
        }

    private:
        [[nodiscard]] adapters::platform::UniqueHandle OpenVolumeHandle(const std::wstring& volumePath);

        [[nodiscard]] domain::Expected<USN_JOURNAL_DATA> QueryUSNJournal(HANDLE hVolume);

        [[nodiscard]] domain::Expected<void> ReadUSNJournal(
            HANDLE hVolume,
            const USN_JOURNAL_DATA& journalData,
            std::vector<MFTFileRecord>& outRecords
        );

        [[nodiscard]] bool ParseUSNRecord(
            const USN_RECORD* record,
            MFTFileRecord& outFileRecord
        );

        [[nodiscard]] std::wstring NormalizeVolumePath(const std::wstring& volumePath);

        [[nodiscard]] std::wstring NormalizeFilePath(const std::wstring& filePath);

        void BuildFilePathMap(const std::vector<MFTFileRecord>& records);

        [[nodiscard]] std::wstring GetFullPath(uint64_t fileRefNumber);

        uint32_t mMaxFilesToScan = 1000000;
        uint32_t mScanTimeoutMs = 30000;
        std::unordered_map<uint64_t, MFTFileRecord> mFileRecordMap;
        std::unordered_map<std::wstring, uint64_t> mPathToRefNumberMap;
    };

}
