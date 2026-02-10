// src/adapters/platform/win32/storage/Win32VolumeService.cpp
#include "Win32VolumeService.h"
#include "../core/Win32HandleFactory.h"
#include "../core/Win32ErrorHandler.h"
#include "../../../../domain/valueobjects/FileSystemType.h"
#include "../../../../domain/valueobjects/DiskSize.h"
#include <algorithm>
#include <cwctype>

namespace winsetup::adapters::platform {

    Win32VolumeService::Win32VolumeService(std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
    {
        if (mLogger) {
            mLogger->Info(L"Win32VolumeService initialized");
        }
    }

    domain::Expected<std::vector<domain::VolumeInfo>> Win32VolumeService::EnumerateVolumes() {
        if (mLogger) {
            mLogger->Debug(L"Enumerating volumes...");
        }

        std::vector<domain::VolumeInfo> volumes;
        volumes.reserve(26);

        wchar_t volumeName[MAX_PATH] = {};
        HANDLE hFind = FindFirstVolumeW(volumeName, MAX_PATH);

        if (hFind == INVALID_HANDLE_VALUE) {
            return domain::Error{
                L"Failed to enumerate volumes",
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        domain::UniqueFindHandle findHandle(
            reinterpret_cast<domain::NativeHandle>(hFind),
            [](domain::NativeHandle handle) noexcept {
                if (handle && handle != reinterpret_cast<domain::NativeHandle>(-1)) {
                    FindVolumeClose(static_cast<HANDLE>(handle));
                }
            }
        );

        int volumeIndex = 0;

        do {
            std::wstring volumePath(volumeName);

            if (volumePath.empty()) {
                continue;
            }

            if (volumePath.back() == L'\\') {
                volumePath.pop_back();
            }

            auto driveLettersResult = GetDriveLetters(volumePath);
            if (!driveLettersResult.HasValue()) {
                if (mLogger) {
                    mLogger->Debug(L"Volume has no drive letters: " + volumePath);
                }
                continue;
            }

            const auto& driveLetters = driveLettersResult.Value();
            if (driveLetters.empty()) {
                continue;
            }

            std::wstring primaryLetter = driveLetters[0];
            if (primaryLetter.length() >= 2) {
                primaryLetter = primaryLetter.substr(0, 2);
            }

            auto labelResult = GetVolumeLabel(volumePath);
            std::wstring label = labelResult.HasValue() ? labelResult.Value() : L"";

            auto fsResult = GetFileSystem(volumePath);
            domain::FileSystemType fileSystem = fsResult.HasValue() ?
                fsResult.Value() : domain::FileSystemType::Unknown;

            auto sizeResult = GetVolumeSize(volumePath);
            domain::DiskSize size = sizeResult.HasValue() ?
                sizeResult.Value() : domain::DiskSize::FromBytes(0);

            auto volumeTypeResult = GetVolumeType(volumePath);
            std::wstring volumeType = volumeTypeResult.HasValue() ?
                volumeTypeResult.Value() : L"";

            domain::VolumeInfo volume(volumeIndex, primaryLetter, label, fileSystem, size);
            volume.SetVolumeType(volumeType);
            volume.SetVolumePath(volumePath);
            volume.SetMounted(IsVolumeMounted(volumePath));

            volumes.push_back(std::move(volume));
            volumeIndex++;

        } while (FindNextVolumeW(static_cast<HANDLE>(findHandle.Get()), volumeName, MAX_PATH));

        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_FILES) {
            if (mLogger) {
                mLogger->Warning(L"FindNextVolume ended with error: " + std::to_wstring(error));
            }
        }

        if (mLogger) {
            mLogger->Info(L"Found " + std::to_wstring(volumes.size()) + L" volumes");
        }

        return volumes;
    }

    domain::Expected<domain::VolumeInfo> Win32VolumeService::GetVolumeInfo(const std::wstring& volumePath) {
        std::wstring normalizedPath = volumePath;

        if (normalizedPath.length() == 2 && normalizedPath[1] == L':') {
            normalizedPath += L"\\";
        }

        auto labelResult = GetVolumeLabel(normalizedPath);
        std::wstring label = labelResult.HasValue() ? labelResult.Value() : L"";

        auto fsResult = GetFileSystem(normalizedPath);
        if (!fsResult.HasValue()) {
            return fsResult.GetError();
        }

        auto sizeResult = GetVolumeSize(normalizedPath);
        if (!sizeResult.HasValue()) {
            return sizeResult.GetError();
        }

        auto volumeTypeResult = GetVolumeType(normalizedPath);
        std::wstring volumeType = volumeTypeResult.HasValue() ?
            volumeTypeResult.Value() : L"";

        std::wstring letter = normalizedPath.length() >= 2 ?
            normalizedPath.substr(0, 2) : L"";

        domain::VolumeInfo volume(
            0,
            letter,
            label,
            fsResult.Value(),
            sizeResult.Value()
        );

        volume.SetVolumeType(volumeType);
        volume.SetVolumePath(normalizedPath);
        volume.SetMounted(IsVolumeMounted(normalizedPath));

        return volume;
    }

    domain::Expected<void> Win32VolumeService::MountVolume(
        const std::wstring& volumePath,
        wchar_t driveLetter)
    {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint += L":\\";

        std::wstring volumeGuid = volumePath;
        if (volumeGuid.back() != L'\\') {
            volumeGuid += L'\\';
        }

        BOOL result = SetVolumeMountPointW(mountPoint.c_str(), volumeGuid.c_str());

        if (!result) {
            return domain::Error{
                L"Failed to mount volume " + volumePath + L" to " + mountPoint,
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        if (mLogger) {
            mLogger->Info(L"Mounted volume " + volumePath + L" to " + mountPoint);
        }

        return domain::Expected<void>{};
    }

    domain::Expected<void> Win32VolumeService::DismountVolume(wchar_t driveLetter) {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint += L":\\";

        BOOL result = DeleteVolumeMountPointW(mountPoint.c_str());

        if (!result) {
            return domain::Error{
                L"Failed to dismount volume from " + mountPoint,
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        if (mLogger) {
            mLogger->Info(L"Dismounted volume from " + mountPoint);
        }

        return domain::Expected<void>{};
    }

    domain::Expected<std::wstring> Win32VolumeService::GetVolumeLabel(const std::wstring& volumePath) {
        wchar_t volumeLabel[MAX_PATH + 1] = {};
        DWORD serialNumber = 0;
        DWORD maxComponentLen = 0;
        DWORD fileSystemFlags = 0;
        wchar_t fileSystemName[MAX_PATH + 1] = {};

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':') {
            path += L"\\";
        }

        BOOL result = GetVolumeInformationW(
            path.c_str(),
            volumeLabel,
            MAX_PATH,
            &serialNumber,
            &maxComponentLen,
            &fileSystemFlags,
            fileSystemName,
            MAX_PATH
        );

        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_NOT_READY || error == ERROR_PATH_NOT_FOUND) {
                return std::wstring{ L"" };
            }
            return domain::Error{
                L"Failed to get volume label for " + volumePath,
                error,
                domain::ErrorCategory::Volume
            };
        }

        return std::wstring(volumeLabel);
    }

    domain::Expected<domain::FileSystemType> Win32VolumeService::GetFileSystem(const std::wstring& volumePath) {
        wchar_t volumeLabel[MAX_PATH + 1] = {};
        DWORD serialNumber = 0;
        DWORD maxComponentLen = 0;
        DWORD fileSystemFlags = 0;
        wchar_t fileSystemName[MAX_PATH + 1] = {};

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':') {
            path += L"\\";
        }

        BOOL result = GetVolumeInformationW(
            path.c_str(),
            volumeLabel,
            MAX_PATH,
            &serialNumber,
            &maxComponentLen,
            &fileSystemFlags,
            fileSystemName,
            MAX_PATH
        );

        if (!result) {
            return domain::Error{
                L"Failed to get file system for " + volumePath,
                GetLastError(),
                domain::ErrorCategory::Volume
            };
        }

        std::wstring fsName(fileSystemName);
        std::transform(fsName.begin(), fsName.end(), fsName.begin(), ::towupper);

        if (fsName == L"NTFS") {
            return domain::FileSystemType::NTFS;
        }
        else if (fsName == L"FAT32") {
            return domain::FileSystemType::FAT32;
        }
        else if (fsName == L"EXFAT") {
            return domain::FileSystemType::exFAT;
        }
        else if (fsName == L"REFS") {
            return domain::FileSystemType::ReFS;
        }

        return domain::FileSystemType::Unknown;
    }

    domain::Expected<domain::DiskSize> Win32VolumeService::GetVolumeSize(const std::wstring& volumePath) {
        ULARGE_INTEGER freeBytesAvailable;
        ULARGE_INTEGER totalBytes;
        ULARGE_INTEGER totalFreeBytes;

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':') {
            path += L"\\";
        }

        BOOL result = GetDiskFreeSpaceExW(
            path.c_str(),
            &freeBytesAvailable,
            &totalBytes,
            &totalFreeBytes
        );

        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_NOT_READY || error == ERROR_PATH_NOT_FOUND) {
                return domain::DiskSize::FromBytes(0);
            }
            return domain::Error{
                L"Failed to get volume size for " + volumePath,
                error,
                domain::ErrorCategory::Volume
            };
        }

        return domain::DiskSize::FromBytes(totalBytes.QuadPart);
    }

    domain::Expected<std::wstring> Win32VolumeService::GetVolumeType(const std::wstring& volumePath) {
        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':') {
            path += L"\\";
        }

        UINT driveType = GetDriveTypeW(path.c_str());

        switch (driveType) {
        case DRIVE_FIXED:
            return std::wstring{ L"Fixed" };
        case DRIVE_REMOVABLE:
            return std::wstring{ L"Removable" };
        case DRIVE_REMOTE:
            return std::wstring{ L"Network" };
        case DRIVE_CDROM:
            return std::wstring{ L"CD-ROM" };
        case DRIVE_RAMDISK:
            return std::wstring{ L"RAM Disk" };
        default:
            return std::wstring{ L"Unknown" };
        }
    }

    domain::Expected<std::vector<std::wstring>> Win32VolumeService::GetDriveLetters(const std::wstring& volumeGuid) {
        std::wstring guid = volumeGuid;
        if (guid.back() != L'\\') {
            guid += L'\\';
        }

        DWORD charCount = MAX_PATH + 1;
        std::vector<wchar_t> buffer(charCount);

        BOOL result = GetVolumePathNamesForVolumeNameW(
            guid.c_str(),
            buffer.data(),
            charCount,
            &charCount
        );

        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_MORE_DATA) {
                buffer.resize(charCount);
                result = GetVolumePathNamesForVolumeNameW(
                    guid.c_str(),
                    buffer.data(),
                    charCount,
                    &charCount
                );
            }

            if (!result) {
                return domain::Error{
                    L"Failed to get drive letters for " + volumeGuid,
                    GetLastError(),
                    domain::ErrorCategory::Volume
                };
            }
        }

        std::vector<std::wstring> driveLetters;
        const wchar_t* ptr = buffer.data();

        while (*ptr != L'\0') {
            std::wstring path(ptr);
            if (!path.empty()) {
                driveLetters.push_back(path);
            }
            ptr += path.length() + 1;
        }

        return driveLetters;
    }

    bool Win32VolumeService::IsVolumeMounted(const std::wstring& volumePath) {
        auto driveLettersResult = GetDriveLetters(volumePath);
        if (!driveLettersResult.HasValue()) {
            return false;
        }

        const auto& driveLetters = driveLettersResult.Value();
        return !driveLetters.empty();
    }

}
