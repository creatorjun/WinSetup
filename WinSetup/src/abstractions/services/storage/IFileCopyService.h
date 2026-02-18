// src/abstractions/services/storage/IFileCopyService.h
#pragma once
#include "domain/primitives/Expected.h"
#include <functional>
#include <string>
#include <cstdint>

namespace winsetup::abstractions {

    struct FileCopyProgress {
        uint64_t     totalBytes = 0;
        uint64_t     copiedBytes = 0;
        uint32_t     totalFiles = 0;
        uint32_t     copiedFiles = 0;
        uint32_t     percentComplete = 0;
        std::wstring currentFile;
    };

    struct FileCopyOptions {
        bool     overwrite = false;
        bool     recursive = true;
        uint32_t threadCount = 0;
        uint32_t bufferSizeKB = 256;
    };

    using FileCopyProgressCallback = std::function<void(const FileCopyProgress&)>;

    class IFileCopyService {
    public:
        virtual ~IFileCopyService() = default;

        [[nodiscard]] virtual winsetup::domain::Expected<void> CopyFile(
            const std::wstring& srcPath,
            const std::wstring& dstPath,
            const FileCopyOptions& options,
            FileCopyProgressCallback progressCallback
        ) = 0;

        [[nodiscard]] virtual winsetup::domain::Expected<void> CopyDirectory(
            const std::wstring& srcDir,
            const std::wstring& dstDir,
            const FileCopyOptions& options,
            FileCopyProgressCallback progressCallback
        ) = 0;

        virtual void Cancel() noexcept = 0;

        [[nodiscard]] virtual bool IsCancelled() const noexcept = 0;

        [[nodiscard]] virtual FileCopyProgress GetLastProgress() const noexcept = 0;
    };

}
