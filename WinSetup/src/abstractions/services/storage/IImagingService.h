// src/abstractions/services/storage/IImagingService.h
#pragma once

#include <domain/primitives/Expected.h>
#include <cstdint>
#include <string>
#include <functional>

namespace winsetup::abstractions {

    enum class CompressionType {
        None,
        XPRESS,
        LZX,
        LZMS
    };

    enum class ImageApplyFlags {
        None = 0,
        Verify = 1 << 0,
        NoAcls = 1 << 1,
        StrictAcls = 1 << 2,
        RpFix = 1 << 3,
        NoRpFix = 1 << 4
    };

    struct ImageInfo {
        uint32_t imageIndex;
        std::wstring name;
        std::wstring description;
        uint64_t totalBytes;
        uint64_t fileCount;
        std::wstring creationTime;
    };

    struct ImageProgress {
        uint64_t completedBytes;
        uint64_t totalBytes;
        uint32_t percentComplete;
        std::wstring currentFile;
    };

    using ProgressCallback = std::function<void(const ImageProgress&)>;

    class IImagingService {
    public:
        virtual ~IImagingService() = default;

        [[nodiscard]] virtual domain::Expected<void> ApplyImage(
            const std::wstring& wimPath,
            uint32_t imageIndex,
            const std::wstring& targetPath,
            ProgressCallback progressCallback = nullptr
        ) = 0;

        [[nodiscard]] virtual domain::Expected<void> CaptureImage(
            const std::wstring& sourcePath,
            const std::wstring& wimPath,
            const std::wstring& imageName,
            const std::wstring& imageDescription,
            CompressionType compression = CompressionType::LZX,
            ProgressCallback progressCallback = nullptr
        ) = 0;

        [[nodiscard]] virtual domain::Expected<std::vector<ImageInfo>>
            GetImageInfo(const std::wstring& wimPath) = 0;

        [[nodiscard]] virtual domain::Expected<void> OptimizeImage(
            const std::wstring& wimPath,
            CompressionType compression = CompressionType::LZX
        ) = 0;

        virtual void SetCompressionLevel(uint32_t level) = 0;
        virtual void SetThreadCount(uint32_t threads) = 0;
        virtual void SetMemoryLimit(uint64_t memoryMB) = 0;
    };

}
