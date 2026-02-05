#include "Win32VolumeService.h"
#include <algorithm>

namespace winsetup::adapters {

    Win32VolumeService::Win32VolumeService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    )
        : textEncoder_(std::move(textEncoder)) {
    }

    domain::Expected<std::vector<domain::VolumeInfo>>
        Win32VolumeService::EnumerateVolumes() const noexcept {
        std::vector<domain::VolumeInfo> volumes;
        volumes.reserve(32);

        wchar_t volumeName[MAX_PATH];
        HANDLE findHandle = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

        if (findHandle == INVALID_HANDLE_VALUE) [[unlikely]] {
            return domain::Expected<std::vector<domain::VolumeInfo>>::Failure(
                CreateErrorFromLastError("FindFirstVolumeW")
            );
        }

        do {
            auto volumeInfoResult = GetVolumeInfo(volumeName);
            if (volumeInfoResult.HasValue()) [[likely]] {
                volumes.push_back(std::move(volumeInfoResult).Value());
            }
        } while (FindNextVolumeW(findHandle, volumeName, ARRAYSIZE(volumeName)));

        FindVolumeClose(findHandle);

        return domain::Expected<std::vector<domain::VolumeInfo>>::Success(
            std::move(volumes)
        );
    }

    domain::Expected<domain::VolumeInfo>
        Win32VolumeService::GetVolumeInfo(const std::wstring& volumePath) const noexcept {
        wchar_t fsName[MAX_PATH];
        wchar_t volumeLabel[MAX_PATH];
        DWORD serialNumber = 0;
        DWORD maxComponentLength = 0;
        DWORD fsFlags = 0;

        BOOL success = GetVolumeInformationW(
            volumePath.c_str(),
            volumeLabel,
            ARRAYSIZE(volumeLabel),
            &serialNumber,
            &maxComponentLength,
            &fsFlags,
            fsName,
            ARRAYSIZE(fsName)
        );

        if (!success) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                CreateErrorFromLastError("GetVolumeInformationW")
            );
        }

        auto fsTypeResult = ParseFileSystemType(fsName);
        domain::FileSystemType fsType = fsTypeResult.HasValue()
            ? fsTypeResult.Value()
            : domain::FileSystemType::Unknown;

        ULARGE_INTEGER freeBytesAvailable;
        ULARGE_INTEGER totalNumberOfBytes;
        ULARGE_INTEGER totalNumberOfFreeBytes;

        success = GetDiskFreeSpaceExW(
            volumePath.c_str(),
            &freeBytesAvailable,
            &totalNumberOfBytes,
            &totalNumberOfFreeBytes
        );

        uint64_t totalSize = success ? totalNumberOfBytes.QuadPart : 0;
        uint64_t freeSize = success ? totalNumberOfFreeBytes.QuadPart : 0;

        auto driveLetterResult = GetVolumeDriveLetter(volumePath);
        std::optional<wchar_t> driveLetter = driveLetterResult.HasValue()
            ? driveLetterResult.Value()
            : std::nullopt;

        domain::VolumeInfo volumeInfo(
            volumePath,
            driveLetter,
            fsType,
            totalSize,
            freeSize
        );

        volumeInfo.SetLabel(volumeLabel);

        return domain::Expected<domain::VolumeInfo>::Success(std::move(volumeInfo));
    }

    domain::Expected<domain::VolumeInfo>
        Win32VolumeService::GetVolumeInfoByDriveLetter(wchar_t driveLetter) const noexcept {
        std::wstring drivePath;
        drivePath.push_back(driveLetter);
        drivePath.append(L":\\");

        wchar_t volumeName[MAX_PATH];
        BOOL success = GetVolumeNameForVolumeMountPointW(
            drivePath.c_str(),
            volumeName,
            ARRAYSIZE(volumeName)
        );

        if (!success) [[unlikely]] {
            return domain::Expected<domain::VolumeInfo>::Failure(
                CreateErrorFromLastError("GetVolumeNameForVolumeMountPointW")
            );
        }

        return GetVolumeInfo(volumeName);
    }

    domain::Result<> Win32VolumeService::AssignDriveLetter(
        const std::wstring& volumePath,
        wchar_t driveLetter
    ) noexcept {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint.append(L":\\");

        BOOL success = SetVolumeMountPointW(mountPoint.c_str(), volumePath.c_str());

        if (!success) [[unlikely]] {
            return domain::Result<>::Failure(
                CreateErrorFromLastError("SetVolumeMountPointW")
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32VolumeService::RemoveDriveLetter(
        wchar_t driveLetter
    ) noexcept {
        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint.append(L":\\");

        BOOL success = DeleteVolumeMountPointW(mountPoint.c_str());

        if (!success) [[unlikely]] {
            return domain::Result<>::Failure(
                CreateErrorFromLastError("DeleteVolumeMountPointW")
            );
        }

        return domain::Result<>::Success();
    }

    domain::Expected<bool> Win32VolumeService::PathExists(
        const std::wstring& path
    ) const noexcept {
        DWORD attributes = GetFileAttributesW(path.c_str());
        return domain::Expected<bool>::Success(attributes != INVALID_FILE_ATTRIBUTES);
    }

    domain::Expected<bool> Win32VolumeService::DirectoryExists(
        const std::wstring& dirPath
    ) const noexcept {
        DWORD attributes = GetFileAttributesW(dirPath.c_str());

        if (attributes == INVALID_FILE_ATTRIBUTES) [[unlikely]] {
            return domain::Expected<bool>::Success(false);
        }

        bool isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        return domain::Expected<bool>::Success(isDirectory);
    }

    domain::Expected<bool> Win32VolumeService::FileExists(
        const std::wstring& filePath
    ) const noexcept {
        DWORD attributes = GetFileAttributesW(filePath.c_str());

        if (attributes == INVALID_FILE_ATTRIBUTES) [[unlikely]] {
            return domain::Expected<bool>::Success(false);
        }

        bool isFile = (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        return domain::Expected<bool>::Success(isFile);
    }

    domain::Expected<domain::FileSystemType>
        Win32VolumeService::ParseFileSystemType(const std::wstring& fsName) const noexcept {
        if (fsName == L"NTFS") {
            return domain::Expected<domain::FileSystemType>::Success(
                domain::FileSystemType::NTFS
            );
        }
        if (fsName == L"FAT32") {
            return domain::Expected<domain::FileSystemType>::Success(
                domain::FileSystemType::FAT32
            );
        }
        if (fsName == L"exFAT") {
            return domain::Expected<domain::FileSystemType>::Success(
                domain::FileSystemType::exFAT
            );
        }
        if (fsName == L"ReFS") {
            return domain::Expected<domain::FileSystemType>::Success(
                domain::FileSystemType::ReFS
            );
        }

        return domain::Expected<domain::FileSystemType>::Success(
            domain::FileSystemType::Unknown
        );
    }

    domain::Expected<std::optional<wchar_t>>
        Win32VolumeService::GetVolumeDriveLetter(const std::wstring& volumePath) const noexcept {
        wchar_t driveStrings[512];
        DWORD length = GetLogicalDriveStringsW(ARRAYSIZE(driveStrings), driveStrings);

        if (length == 0 || length > ARRAYSIZE(driveStrings)) [[unlikely]] {
            return domain::Expected<std::optional<wchar_t>>::Success(std::nullopt);
        }

        wchar_t* drive = driveStrings;
        while (*drive) {
            wchar_t volumeName[MAX_PATH];
            BOOL success = GetVolumeNameForVolumeMountPointW(
                drive,
                volumeName,
                ARRAYSIZE(volumeName)
            );

            if (success && volumePath == volumeName) [[unlikely]] {
                return domain::Expected<std::optional<wchar_t>>::Success(
                    std::make_optional(drive[0])
                );
            }

            drive += wcslen(drive) + 1;
        }

        return domain::Expected<std::optional<wchar_t>>::Success(std::nullopt);
    }

    domain::Error Win32VolumeService::CreateErrorFromLastError(
        std::string_view operation
    ) const noexcept {
        const DWORD errorCode = ::GetLastError();

        std::string message = "Win32VolumeService: ";
        message.append(operation);
        message.append(" failed with error code: ");
        message.append(std::to_string(errorCode));

        return domain::Error(std::move(message), static_cast<int>(errorCode));
    }

    std::unique_ptr<abstractions::IVolumeService> CreateVolumeService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    ) {
        return std::make_unique<Win32VolumeService>(std::move(textEncoder));
    }

}
