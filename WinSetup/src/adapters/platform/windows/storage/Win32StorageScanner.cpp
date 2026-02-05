#include "Win32StorageScanner.h"
#include <algorithm>

namespace winsetup::adapters {

    Win32StorageScanner::Win32StorageScanner(
        std::shared_ptr<abstractions::IVolumeService> volumeService,
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : volumeService_(std::move(volumeService))
        , textEncoder_(std::move(textEncoder))
        , logger_(std::move(logger)) {
        if (logger_) {
            logger_->Log(abstractions::LogLevel::Info, L"Win32StorageScanner initialized");
        }
    }

    domain::Expected<abstractions::ScanResult>
        Win32StorageScanner::ScanVolume(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile
        ) const noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Starting volume scan for user profile: " + userProfile
            );
        }

        if (!volume.HasDriveLetter()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Warning,
                    L"Volume has no drive letter, cannot scan"
                );
            }
            return domain::Expected<abstractions::ScanResult>::Failure(
                domain::Error("Volume has no drive letter")
            );
        }

        std::wstring basePath = volume.GetDriveLetterString();
        if (basePath.empty()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Invalid drive letter for volume"
                );
            }
            return domain::Expected<abstractions::ScanResult>::Failure(
                domain::Error("Invalid drive letter")
            );
        }

        basePath.append(L"\\");

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Scanning volume at: " + basePath
            );
        }

        abstractions::ScanResult result;

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Checking for Windows directory"
            );
        }
        auto windowsDirResult = volumeService_->DirectoryExists(basePath + L"Windows");
        if (windowsDirResult.HasValue() && windowsDirResult.Value()) [[likely]] {
            result.hasWindowsDirectory = true;
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Windows directory found"
                );
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Checking for System32 directory"
            );
        }
        auto system32DirResult = volumeService_->DirectoryExists(
            basePath + L"Windows\\System32"
        );
        if (system32DirResult.HasValue() && system32DirResult.Value()) [[likely]] {
            result.hasSystem32Directory = true;
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"System32 directory found"
                );
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Checking for Users directory"
            );
        }
        auto usersDirResult = volumeService_->DirectoryExists(basePath + L"Users");
        if (usersDirResult.HasValue() && usersDirResult.Value()) [[likely]] {
            result.hasUsersDirectory = true;
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Users directory found"
                );
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Checking for Program Files directory"
            );
        }
        auto programFilesDirResult = volumeService_->DirectoryExists(
            basePath + L"Program Files"
        );
        if (programFilesDirResult.HasValue() && programFilesDirResult.Value()) [[likely]] {
            result.hasProgramFilesDirectory = true;
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"Program Files directory found"
                );
            }
        }

        result.score = CalculateSystemScore(
            result.hasWindowsDirectory,
            result.hasSystem32Directory,
            result.hasUsersDirectory,
            result.hasProgramFilesDirectory
        );

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Initial system score: " + std::to_wstring(result.score)
            );
        }

        if (result.score < 50) {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Debug,
                    L"System score below threshold, searching for user data directories"
                );
            }

            auto userDirsResult = FindUserDataDirectories(volume, userProfile);
            if (userDirsResult.HasValue()) [[likely]] {
                int dataScore = CalculateDataScore(userDirsResult.Value());
                if (logger_) {
                    logger_->Log(
                        abstractions::LogLevel::Debug,
                        L"User data score: " + std::to_wstring(dataScore)
                    );
                }
                result.score = std::max(result.score, dataScore);
            }
        }

        if (logger_) {
            std::wstring volumeType;
            if (result.IsLikelySystemVolume()) {
                volumeType = L"System Volume";
            }
            else if (result.IsLikelyDataVolume()) {
                volumeType = L"Data Volume";
            }
            else {
                volumeType = L"Unknown Volume Type";
            }

            logger_->Log(
                abstractions::LogLevel::Info,
                L"Volume scan complete - " + volumeType +
                L", Final score: " + std::to_wstring(result.score) +
                L" (Windows:" + (result.hasWindowsDirectory ? L"Yes" : L"No") +
                L", System32:" + (result.hasSystem32Directory ? L"Yes" : L"No") +
                L", Users:" + (result.hasUsersDirectory ? L"Yes" : L"No") +
                L", ProgramFiles:" + (result.hasProgramFilesDirectory ? L"Yes" : L"No") + L")"
            );
        }

        return domain::Expected<abstractions::ScanResult>::Success(result);
    }

    domain::Expected<std::vector<std::wstring>>
        Win32StorageScanner::FindUserDataDirectories(
            const domain::VolumeInfo& volume,
            const std::wstring& userProfile
        ) const noexcept {
        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Finding user data directories for profile: " + userProfile
            );
        }

        if (!volume.HasDriveLetter()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Warning,
                    L"Volume has no drive letter"
                );
            }
            return domain::Expected<std::vector<std::wstring>>::Failure(
                domain::Error("Volume has no drive letter")
            );
        }

        std::wstring basePath = volume.GetDriveLetterString();
        if (basePath.empty()) [[unlikely]] {
            if (logger_) {
                logger_->Log(
                    abstractions::LogLevel::Error,
                    L"Invalid drive letter"
                );
            }
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

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Debug,
                L"Checking " + std::to_wstring(commonUserDirs.size()) + L" common user directories"
            );
        }

        int foundCount = 0;
        for (const auto& dir : commonUserDirs) {
            std::wstring fullPath = basePath + dir;
            auto existsResult = volumeService_->DirectoryExists(fullPath);

            if (existsResult.HasValue() && existsResult.Value()) [[likely]] {
                userDirs.push_back(dir);
                foundCount++;
                if (logger_) {
                    logger_->Log(
                        abstractions::LogLevel::Debug,
                        L"Found user directory: " + dir
                    );
                }
            }
        }

        if (logger_) {
            logger_->Log(
                abstractions::LogLevel::Info,
                L"Found " + std::to_wstring(foundCount) + L" user data directories"
            );
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
        std::shared_ptr<abstractions::ITextEncoder> textEncoder,
        std::shared_ptr<abstractions::ILogger> logger
    ) {
        return std::make_unique<Win32StorageScanner>(
            std::move(volumeService),
            std::move(textEncoder),
            std::move(logger)
        );
    }

}
