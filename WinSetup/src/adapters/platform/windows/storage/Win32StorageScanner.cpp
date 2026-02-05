#include "Win32StorageScanner.h"
#include <algorithm>

namespace winsetup::adapters {

    Win32StorageScanner::Win32StorageScanner(
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    )
        : volumeService_(std::move(volumeService))
        , textEncoder_(std::move(textEncoder)) {
    }

    domain::Expected<abstractions::ScanResult>
        Win32StorageScanner::ScanVolume(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile
        ) const noexcept {
        if (!volume.HasDriveLetter()) [[unlikely]] {
            return domain::Expected<abstractions::ScanResult>::Failure(
                domain::Error("Volume has no drive letter")
            );
        }

        std::wstring basePath = volume.GetDriveLetterString();
        if (basePath.empty()) [[unlikely]] {
            return domain::Expected<abstractions::ScanResult>::Failure(
                domain::Error("Invalid drive letter")
            );
        }

        basePath.append(L"\\");

        abstractions::ScanResult result;

        auto windowsDirResult = volumeService_->DirectoryExists(basePath + L"Windows");
        if (windowsDirResult.HasValue() && windowsDirResult.Value()) [[likely]] {
            result.hasWindowsDirectory = true;
        }

        auto system32DirResult = volumeService_->DirectoryExists(
            basePath + L"Windows\\System32"
        );
        if (system32DirResult.HasValue() && system32DirResult.Value()) [[likely]] {
            result.hasSystem32Directory = true;
        }

        auto usersDirResult = volumeService_->DirectoryExists(basePath + L"Users");
        if (usersDirResult.HasValue() && usersDirResult.Value()) [[likely]] {
            result.hasUsersDirectory = true;
        }

        auto programFilesDirResult = volumeService_->DirectoryExists(
            basePath + L"Program Files"
        );
        if (programFilesDirResult.HasValue() && programFilesDirResult.Value()) [[likely]] {
            result.hasProgramFilesDirectory = true;
        }

        result.score = CalculateSystemScore(
            result.hasWindowsDirectory,
            result.hasSystem32Directory,
            result.hasUsersDirectory,
            result.hasProgramFilesDirectory
        );

        if (result.score < 50) {
            auto userDirsResult = FindUserDataDirectories(volume, userProfile);
            if (userDirsResult.HasValue()) [[likely]] {
                int dataScore = CalculateDataScore(userDirsResult.Value());
                result.score = std::max(result.score, dataScore);
            }
        }

        return domain::Expected<abstractions::ScanResult>::Success(result);
    }

    domain::Expected<std::vector<std::wstring>>
        Win32StorageScanner::FindUserDataDirectories(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile
        ) const noexcept {
        if (!volume.HasDriveLetter()) [[unlikely]] {
            return domain::Expected<std::vector<std::wstring>>::Failure(
                domain::Error("Volume has no drive letter")
            );
        }

        std::wstring basePath = volume.GetDriveLetterString();
        if (basePath.empty()) [[unlikely]] {
            return domain::Expected<std::vector<std::wstring>>::Failure(
                domain::Error("Invalid drive letter")
            );
        }

        basePath.append(L"\\");

        std::vector<std::wstring> userDirs;
        userDirs.reserve(10);

        const std::vector<std::wstring> commonUserDirs = {
            userProfile + L"\\Desktop",
            userProfile + L"\\Documents",
            userProfile + L"\\Downloads",
            userProfile + L"\\Pictures",
            userProfile + L"\\Videos",
            userProfile + L"\\Music",
            userProfile + L"\\AppData",
            L"Users\\" + userProfile + L"\\Desktop",
            L"Users\\" + userProfile + L"\\Documents",
            L"Users\\" + userProfile + L"\\Downloads"
        };

        for (const auto& dir : commonUserDirs) {
            std::wstring fullPath = basePath + dir;
            auto existsResult = volumeService_->DirectoryExists(fullPath);

            if (existsResult.HasValue() && existsResult.Value()) [[likely]] {
                userDirs.push_back(dir);
            }
        }

        return domain::Expected<std::vector<std::wstring>>::Success(
            std::move(userDirs)
        );
    }

    int Win32StorageScanner::CalculateSystemScore(
        bool hasWindows,
        bool hasSystem32,
        bool hasUsers,
        bool hasProgramFiles
    ) const noexcept {
        int score = 0;

        if (hasWindows) score += 30;
        if (hasSystem32) score += 40;
        if (hasUsers) score += 20;
        if (hasProgramFiles) score += 10;

        return score;
    }

    int Win32StorageScanner::CalculateDataScore(
        const std::vector<std::wstring>& userDirs
    ) const noexcept {
        if (userDirs.empty()) return 0;

        int score = 0;

        for (const auto& dir : userDirs) {
            if (dir.find(L"Desktop") != std::wstring::npos) score += 15;
            else if (dir.find(L"Documents") != std::wstring::npos) score += 15;
            else if (dir.find(L"Downloads") != std::wstring::npos) score += 10;
            else if (dir.find(L"AppData") != std::wstring::npos) score += 5;
            else score += 5;
        }

        return std::min(score, 80);
    }

    std::unique_ptr<abstractions::IStorageScanner> CreateStorageScanner(
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::ITextEncoder> textEncoder
    ) {
        return std::make_unique<Win32StorageScanner>(
            std::move(volumeService),
            std::move(textEncoder)
        );
    }

}
