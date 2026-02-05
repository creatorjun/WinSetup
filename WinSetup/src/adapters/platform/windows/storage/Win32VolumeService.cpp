#include "Win32VolumeService.h"
#include <algorithm>
#include "../../../../abstractions/logging/ILogger.h"
#include "../../../../domain/primitives/LogLevel.h"

namespace winsetup::adapters {

    Win32VolumeService::Win32VolumeService(
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Win32VolumeService initialized");
        }
    }

    domain::Expected<std::vector<domain::VolumeInfo>>
        Win32VolumeService::EnumerateVolumes() const noexcept {
        if (logger_) {
            logger_->Log(domain::LogLevel::Info, L"Starting volume enumeration");
        }

        std::vector<domain::VolumeInfo> volumes;
        volumes.reserve(32);

        wchar_t volumeName[MAX_PATH];
        HANDLE findHandle = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

        if (findHandle == INVALID_HANDLE_VALUE) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"FindFirstVolumeW failed with error " + std::to_wstring(errorCode)
                );
            }
            return domain::Expected<std::vector<domain::VolumeInfo>>::Failure(
                CreateErrorFromLastError("FindFirstVolumeW")
            );
        }

        int volumeCount = 0;
        do {
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Debug,
                    L"Processing volume: " + std::wstring(volumeName)
                );
            }

            auto volumeInfoResult = GetVolumeInfo(volumeName);
            if (volumeInfoResult.HasValue()) [[likely]] {
                volumes.push_back(std::move(volumeInfoResult).Value());
                volumeCount++;
            }
            else {
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Warning,
                        L"Failed to get info for volume: " + std::wstring(volumeName)
                    );
                }
            }
        } while (FindNextVolumeW(findHandle, volumeName, ARRAYSIZE(volumeName)));

        FindVolumeClose(findHandle);

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Volume enumeration completed: " + std::to_wstring(volumeCount) + L" volumes found"
            );
        }

        return domain::Expected<std::vector<domain::VolumeInfo>>::Success(
            std::move(volumes)
        );
    }

    domain::Expected<domain::VolumeInfo>
        Win32VolumeService::GetVolumeInfo(const std::wstring& volumePath) const noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Getting volume information for: " + volumePath
            );
        }

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
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Warning,
                    L"GetVolumeInformationW failed for " + volumePath +
                    L", error " + std::to_wstring(errorCode)
                );
            }
            return domain::Expected<domain::VolumeInfo>::Failure(
                CreateErrorFromLastError("GetVolumeInformationW")
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Volume label: " + std::wstring(volumeLabel) +
                L", File system: " + std::wstring(fsName)
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

        if (success && logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Total size: " + std::to_wstring(totalSize / (1024 * 1024 * 1024)) +
                L" GB, Free: " + std::to_wstring(freeSize / (1024 * 1024 * 1024)) + L" GB"
            );
        }

        auto driveLetterResult = GetVolumeDriveLetter(volumePath);
        std::optional<wchar_t> driveLetter = driveLetterResult.HasValue()
            ? driveLetterResult.Value()
            : std::nullopt;

        if (driveLetter.has_value() && logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Drive letter: " + std::wstring(1, driveLetter.value()) + L":"
            );
        }

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
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Getting volume info for drive letter: " + std::wstring(1, driveLetter) + L":"
            );
        }

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
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"GetVolumeNameForVolumeMountPointW failed for " + drivePath +
                    L", error " + std::to_wstring(errorCode)
                );
            }
            return domain::Expected<domain::VolumeInfo>::Failure(
                CreateErrorFromLastError("GetVolumeNameForVolumeMountPointW")
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Volume path resolved: " + std::wstring(volumeName)
            );
        }

        return GetVolumeInfo(volumeName);
    }

    domain::Result<> Win32VolumeService::AssignDriveLetter(
        const std::wstring& volumePath,
        wchar_t driveLetter
    ) noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Assigning drive letter " + std::wstring(1, driveLetter) +
                L": to volume " + volumePath
            );
        }

        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint.append(L":\\");

        BOOL success = SetVolumeMountPointW(mountPoint.c_str(), volumePath.c_str());

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"SetVolumeMountPointW failed, error " + std::to_wstring(errorCode)
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("SetVolumeMountPointW")
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Drive letter assigned successfully"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Result<> Win32VolumeService::RemoveDriveLetter(
        wchar_t driveLetter
    ) noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Removing drive letter: " + std::wstring(1, driveLetter) + L":"
            );
        }

        std::wstring mountPoint;
        mountPoint.push_back(driveLetter);
        mountPoint.append(L":\\");

        BOOL success = DeleteVolumeMountPointW(mountPoint.c_str());

        if (!success) [[unlikely]] {
            const DWORD errorCode = ::GetLastError();
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Error,
                    L"DeleteVolumeMountPointW failed for " + mountPoint +
                    L", error " + std::to_wstring(errorCode)
                );
            }
            return domain::Result<>::Failure(
                CreateErrorFromLastError("DeleteVolumeMountPointW")
            );
        }

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Info,
                L"Drive letter removed successfully"
            );
        }

        return domain::Result<>::Success();
    }

    domain::Expected<bool> Win32VolumeService::PathExists(
        const std::wstring& path
    ) const noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Checking path existence: " + path
            );
        }

        DWORD attributes = GetFileAttributesW(path.c_str());
        bool exists = attributes != INVALID_FILE_ATTRIBUTES;

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                path + (exists ? L" exists" : L" does not exist")
            );
        }

        return domain::Expected<bool>::Success(exists);
    }

    domain::Expected<bool> Win32VolumeService::DirectoryExists(
        const std::wstring& dirPath
    ) const noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Checking directory existence: " + dirPath
            );
        }

        DWORD attributes = GetFileAttributesW(dirPath.c_str());

        if (attributes == INVALID_FILE_ATTRIBUTES) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Debug,
                    dirPath + L" does not exist"
                );
            }
            return domain::Expected<bool>::Success(false);
        }

        bool isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                dirPath + (isDirectory ? L" is a directory" : L" is not a directory")
            );
        }

        return domain::Expected<bool>::Success(isDirectory);
    }

    domain::Expected<bool> Win32VolumeService::FileExists(
        const std::wstring& filePath
    ) const noexcept {
        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                L"Checking file existence: " + filePath
            );
        }

        DWORD attributes = GetFileAttributesW(filePath.c_str());

        if (attributes == INVALID_FILE_ATTRIBUTES) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Debug,
                    filePath + L" does not exist"
                );
            }
            return domain::Expected<bool>::Success(false);
        }

        bool isFile = (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Debug,
                filePath + (isFile ? L" is a file" : L" is not a file")
            );
        }

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

        if (logger_) {
            logger_->Log(
                domain::LogLevel::Warning,
                L"Unknown file system type: " + fsName
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
            if (logger_) {
                logger_->Log(
                    domain::LogLevel::Warning,
                    L"GetLogicalDriveStringsW failed or buffer too small"
                );
            }
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
                if (logger_) {
                    logger_->Log(
                        domain::LogLevel::Debug,
                        L"Found drive letter " + std::wstring(1, drive[0]) +
                        L": for volume " + volumePath
                    );
                }
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
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    ) {
        return std::make_unique<Win32VolumeService>(std::move(textEncoder), std::move(logger));
    }

}
