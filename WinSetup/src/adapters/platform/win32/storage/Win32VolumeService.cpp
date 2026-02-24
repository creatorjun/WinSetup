#include "Win32VolumeService.h"
#include "adapters/platform/win32/core/Win32HandleFactory.h"
#include "adapters/platform/win32/core/Win32ErrorHandler.h"
#include "domain/valueobjects/FileSystemType.h"
#include "domain/valueobjects/DiskSize.h"
#include <algorithm>
#include <cwctype>

namespace winsetup::adapters::platform {

    Win32VolumeService::Win32VolumeService(std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger))
    {
        if (mLogger)
            mLogger->Info(L"Win32VolumeService initialized");
    }

    domain::Expected<std::vector<domain::VolumeInfo>> Win32VolumeService::EnumerateVolumes()
    {
        if (mLogger)
            mLogger->Debug(L"Enumerating volumes...");

        std::vector<domain::VolumeInfo> volumes;
        volumes.reserve(26);

        wchar_t volumeName[MAX_PATH];
        HANDLE hFind = FindFirstVolumeW(volumeName, MAX_PATH);
        if (hFind == INVALID_HANDLE_VALUE)
            return domain::Error(L"Failed to enumerate volumes", GetLastError(), domain::ErrorCategory::Volume);

        auto findHandle = Win32HandleFactory::MakeFindVolumeHandle(hFind);

        int volumeIndex = 0;
        do {
            std::wstring volumePath(volumeName);
            if (volumePath.empty())
                continue;

            if (volumePath.back() == L'\\')
                volumePath.pop_back();

            std::wstring displayLetter;
            auto driveLettersResult = GetDriveLetters(volumePath);
            if (driveLettersResult.HasValue() && !driveLettersResult.Value().empty()) {
                const auto& first = driveLettersResult.Value()[0];
                displayLetter = first.length() >= 2 ? first.substr(0, 2) : first;
            }
            else {
                displayLetter = volumePath;
            }

            auto labelResult = GetVolumeLabel(volumePath);
            std::wstring label = labelResult.HasValue() ? labelResult.Value() : L"";

            auto fsResult = GetFileSystem(volumePath);
            domain::FileSystemType fileSystem = fsResult.HasValue() ? fsResult.Value() : domain::FileSystemType::Unknown;

            auto sizeResult = GetVolumeSize(volumePath);
            domain::DiskSize size = sizeResult.HasValue() ? sizeResult.Value() : domain::DiskSize::FromBytes(0);

            auto volumeTypeResult = GetVolumeType(volumePath);
            std::wstring volumeType = volumeTypeResult.HasValue() ? volumeTypeResult.Value() : L"";

            domain::VolumeInfo volume(volumeIndex, displayLetter, label, fileSystem, size);
            volume.SetVolumeType(volumeType);
            volume.SetVolumePath(volumePath);
            volume.SetMounted(IsVolumeMounted(volumePath));
            volumes.push_back(std::move(volume));
            volumeIndex++;

        } while (FindNextVolumeW(Win32HandleFactory::ToWin32FindHandle(findHandle), volumeName, MAX_PATH));

        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_FILES) {
            if (mLogger)
                mLogger->Warning(L"FindNextVolume ended with error: " + std::to_wstring(error));
        }

        if (mLogger)
            mLogger->Info(L"Found " + std::to_wstring(volumes.size()) + L" volumes");

        return volumes;
    }

    domain::Expected<domain::VolumeInfo> Win32VolumeService::GetVolumeInfo(const std::wstring& volumePath)
    {
        std::wstring normalizedPath = volumePath;
        if (normalizedPath.length() == 2 && normalizedPath[1] == L':')
            normalizedPath += L'\\';

        auto labelResult = GetVolumeLabel(normalizedPath);
        std::wstring label = labelResult.HasValue() ? labelResult.Value() : L"";

        auto fsResult = GetFileSystem(normalizedPath);
        if (!fsResult.HasValue())
            return fsResult.GetError();

        auto sizeResult = GetVolumeSize(normalizedPath);
        if (!sizeResult.HasValue())
            return sizeResult.GetError();

        auto volumeTypeResult = GetVolumeType(normalizedPath);
        std::wstring volumeType = volumeTypeResult.HasValue() ? volumeTypeResult.Value() : L"";

        std::wstring letter = normalizedPath.length() >= 2 ? normalizedPath.substr(0, 2) : L"";

        domain::VolumeInfo volume(0, letter, label, fsResult.Value(), sizeResult.Value());
        volume.SetVolumeType(volumeType);
        volume.SetVolumePath(normalizedPath);
        volume.SetMounted(IsVolumeMounted(normalizedPath));
        return volume;
    }

    domain::Expected<void> Win32VolumeService::MountVolume(
        const std::wstring& volumeGuid,
        wchar_t driveLetter)
    {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint += L":\\";

        std::wstring guid = volumeGuid;
        if (!guid.empty() && guid.back() != L'\\')
            guid += L'\\';

        if (!SetVolumeMountPointW(mountPoint.c_str(), guid.c_str())) {
            const DWORD err = GetLastError();
            if (mLogger)
                mLogger->Error(L"MountVolume failed: " + guid + L" -> " + mountPoint
                    + L" error=" + std::to_wstring(err));
            return domain::Error(
                L"Failed to mount volume " + guid + L" to " + mountPoint,
                err,
                domain::ErrorCategory::Volume);
        }

        if (mLogger)
            mLogger->Info(L"MountVolume: " + guid + L" -> " + mountPoint);

        return domain::Expected<void>();
    }

    domain::Expected<void> Win32VolumeService::DismountVolume(wchar_t driveLetter)
    {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint += L":\\";

        if (!DeleteVolumeMountPointW(mountPoint.c_str())) {
            const DWORD err = GetLastError();
            if (mLogger)
                mLogger->Error(L"DismountVolume failed: " + mountPoint
                    + L" error=" + std::to_wstring(err));
            return domain::Error(
                L"Failed to dismount volume from " + mountPoint,
                err,
                domain::ErrorCategory::Volume);
        }

        if (mLogger)
            mLogger->Info(L"DismountVolume: released " + mountPoint);

        return domain::Expected<void>();
    }

    domain::Expected<std::wstring> Win32VolumeService::GetVolumeLabel(const std::wstring& volumePath)
    {
        wchar_t volumeLabel[MAX_PATH + 1];
        DWORD serialNumber = 0, maxComponentLen = 0, fileSystemFlags = 0;
        wchar_t fileSystemName[MAX_PATH + 1];

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':')
            path += L'\\';
        else if (!path.empty() && path.back() != L'\\')
            path += L'\\';

        if (!GetVolumeInformationW(path.c_str(), volumeLabel, MAX_PATH,
            &serialNumber, &maxComponentLen,
            &fileSystemFlags, fileSystemName, MAX_PATH)) {
            DWORD error = GetLastError();
            if (error == ERROR_NOT_READY || error == ERROR_PATH_NOT_FOUND)
                return std::wstring(L"");
            return domain::Error(L"Failed to get volume label for " + volumePath,
                error, domain::ErrorCategory::Volume);
        }
        return std::wstring(volumeLabel);
    }

    domain::Expected<domain::FileSystemType> Win32VolumeService::GetFileSystem(const std::wstring& volumePath)
    {
        wchar_t volumeLabel[MAX_PATH + 1];
        DWORD serialNumber = 0, maxComponentLen = 0, fileSystemFlags = 0;
        wchar_t fileSystemName[MAX_PATH + 1];

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':')
            path += L'\\';
        else if (!path.empty() && path.back() != L'\\')
            path += L'\\';

        if (!GetVolumeInformationW(path.c_str(), volumeLabel, MAX_PATH,
            &serialNumber, &maxComponentLen,
            &fileSystemFlags, fileSystemName, MAX_PATH))
            return domain::Error(L"Failed to get file system for " + volumePath,
                GetLastError(), domain::ErrorCategory::Volume);

        std::wstring fsName(fileSystemName);
        std::transform(fsName.begin(), fsName.end(), fsName.begin(), towupper);

        if (fsName == L"NTFS")   return domain::FileSystemType::NTFS;
        if (fsName == L"FAT32")  return domain::FileSystemType::FAT32;
        if (fsName == L"EXFAT")  return domain::FileSystemType::exFAT;
        if (fsName == L"REFS")   return domain::FileSystemType::ReFS;
        return domain::FileSystemType::Unknown;
    }

    domain::Expected<domain::DiskSize> Win32VolumeService::GetVolumeSize(const std::wstring& volumePath)
    {
        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':')
            path += L'\\';
        else if (!path.empty() && path.back() != L'\\')
            path += L'\\';

        if (!GetDiskFreeSpaceExW(path.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            DWORD error = GetLastError();
            if (error == ERROR_NOT_READY || error == ERROR_PATH_NOT_FOUND)
                return domain::DiskSize::FromBytes(0);
            return domain::Error(L"Failed to get volume size for " + volumePath,
                error, domain::ErrorCategory::Volume);
        }
        return domain::DiskSize::FromBytes(totalBytes.QuadPart);
    }

    domain::Expected<std::wstring> Win32VolumeService::GetVolumeType(const std::wstring& volumePath)
    {
        std::wstring path = volumePath;
        if (path.length() == 2 && path[1] == L':')
            path += L'\\';
        else if (!path.empty() && path.back() != L'\\')
            path += L'\\';

        UINT driveType = GetDriveTypeW(path.c_str());
        switch (driveType) {
        case DRIVE_FIXED:     return std::wstring(L"Fixed");
        case DRIVE_REMOVABLE: return std::wstring(L"Removable");
        case DRIVE_REMOTE:    return std::wstring(L"Network");
        case DRIVE_CDROM:     return std::wstring(L"CD-ROM");
        case DRIVE_RAMDISK:   return std::wstring(L"RAM Disk");
        default:              return std::wstring(L"Unknown");
        }
    }

    domain::Expected<std::vector<std::wstring>> Win32VolumeService::GetDriveLetters(
        const std::wstring& volumeGuid)
    {
        std::wstring guid = volumeGuid;
        if (!guid.empty() && guid.back() != L'\\')
            guid += L'\\';

        DWORD charCount = MAX_PATH + 1;
        std::vector<wchar_t> buffer(charCount);

        if (!GetVolumePathNamesForVolumeNameW(guid.c_str(), buffer.data(), charCount, &charCount)) {
            DWORD error = GetLastError();
            if (error == ERROR_MORE_DATA) {
                buffer.resize(charCount);
                if (!GetVolumePathNamesForVolumeNameW(guid.c_str(), buffer.data(), charCount, &charCount))
                    return domain::Error(L"Failed to get drive letters for " + volumeGuid,
                        GetLastError(), domain::ErrorCategory::Volume);
            }
            else {
                return domain::Error(L"Failed to get drive letters for " + volumeGuid,
                    error, domain::ErrorCategory::Volume);
            }
        }

        std::vector<std::wstring> driveLetters;
        const wchar_t* ptr = buffer.data();
        while (ptr && *ptr != L'\0') {
            std::wstring path(ptr);
            if (!path.empty())
                driveLetters.push_back(path);
            ptr += path.length() + 1;
        }
        return driveLetters;
    }

    bool Win32VolumeService::IsVolumeMounted(const std::wstring& volumePath)
    {
        auto driveLettersResult = GetDriveLetters(volumePath);
        if (!driveLettersResult.HasValue())
            return false;
        return !driveLettersResult.Value().empty();
    }

} // namespace winsetup::adapters::platform
