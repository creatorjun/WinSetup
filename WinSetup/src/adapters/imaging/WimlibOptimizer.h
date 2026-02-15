// src/adapters/imaging/WimlibOptimizer.h
#pragma once

#include <abstractions/services/storage/IImagingService.h>
#include <domain/primitives/Expected.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <cstdint>
#include <memory>
#include <atomic>

struct WIMStruct;

namespace winsetup::adapters {

    enum class OptimizationLevel {
        Fast = 1,
        Normal = 6,
        Best = 12,
        Ultra = 20
    };

    struct WimlibOptimizerConfig {
        OptimizationLevel level;
        uint32_t maxThreads;
        uint64_t memoryLimitMB;
        uint32_t chunkSizeKB;
        bool enableSolidCompression;
        bool enableDeduplication;

        WimlibOptimizerConfig()
            : level(OptimizationLevel::Normal)
            , maxThreads(0)
            , memoryLimitMB(2048)
            , chunkSizeKB(32)
            , enableSolidCompression(false)
            , enableDeduplication(true)
        {
        }
    };

    struct WimlibOperationStats {
        uint64_t totalBytes;
        uint64_t compressedBytes;
        uint64_t processedFiles;
        double compressionRatio;
        double elapsedSeconds;
        uint64_t peakMemoryMB;

        WimlibOperationStats()
            : totalBytes(0)
            , compressedBytes(0)
            , processedFiles(0)
            , compressionRatio(0.0)
            , elapsedSeconds(0.0)
            , peakMemoryMB(0)
        {
        }
    };

    class WimlibOptimizer : public abstractions::IImagingService {
    public:
        WimlibOptimizer();
        explicit WimlibOptimizer(const WimlibOptimizerConfig& config);
        ~WimlibOptimizer() override;

        WimlibOptimizer(const WimlibOptimizer&) = delete;
        WimlibOptimizer& operator=(const WimlibOptimizer&) = delete;

        [[nodiscard]] domain::Expected<void> Initialize();

        [[nodiscard]] domain::Expected<void> ApplyImage(
            const std::wstring& wimPath,
            uint32_t imageIndex,
            const std::wstring& targetPath,
            abstractions::ProgressCallback progressCallback = nullptr
        ) override;

        [[nodiscard]] domain::Expected<void> CaptureImage(
            const std::wstring& sourcePath,
            const std::wstring& wimPath,
            const std::wstring& imageName,
            const std::wstring& imageDescription,
            abstractions::CompressionType compression = abstractions::CompressionType::LZX,
            abstractions::ProgressCallback progressCallback = nullptr
        ) override;

        [[nodiscard]] domain::Expected<std::vector<abstractions::ImageInfo>>
            GetImageInfo(const std::wstring& wimPath) override;

        [[nodiscard]] domain::Expected<void> OptimizeImage(
            const std::wstring& wimPath,
            abstractions::CompressionType compression = abstractions::CompressionType::LZX
        ) override;

        void SetCompressionLevel(uint32_t level) override;
        void SetThreadCount(uint32_t threads) override;
        void SetMemoryLimit(uint64_t memoryMB) override;

        void SetConfig(const WimlibOptimizerConfig& config);

        [[nodiscard]] const WimlibOptimizerConfig& GetConfig() const noexcept {
            return mConfig;
        }

        [[nodiscard]] domain::Expected<void> OptimizeCapture(
            WIMStruct* wim,
            int compressionType
        );

        [[nodiscard]] domain::Expected<void> OptimizeExtract(
            WIMStruct* wim
        );

        [[nodiscard]] const WimlibOperationStats& GetLastStats() const noexcept {
            return mLastStats;
        }

        void SetProgressCallback(void* callback, void* context);

        [[nodiscard]] uint32_t CalculateOptimalThreadCount() const;
        [[nodiscard]] uint64_t CalculateOptimalMemoryLimit() const;
        [[nodiscard]] uint32_t CalculateOptimalChunkSize(uint64_t estimatedSizeBytes) const;

    private:
        [[nodiscard]] domain::Expected<void> AllocateMemoryPool();
        void ReleaseMemoryPool();
        [[nodiscard]] domain::Expected<void> ConfigureThreadPool();
        [[nodiscard]] domain::Expected<void> ApplyCompressionSettings(WIMStruct* wim, int compressionType);
        [[nodiscard]] domain::Expected<void> ApplyExtractionSettings(WIMStruct* wim);
        void UpdateStats(const void* info);
        [[nodiscard]] uint64_t GetCurrentMemoryUsage() const;

        WimlibOptimizerConfig mConfig;
        WimlibOperationStats mLastStats;

        void* mMemoryPool;
        size_t mMemoryPoolSize;

        void* mProgressCallback;
        void* mProgressContext;

        std::atomic<bool> mInitialized;
        std::atomic<uint64_t> mPeakMemory;

        platform::UniqueHandle mJobObject;

        static constexpr uint32_t MIN_CHUNK_SIZE_KB = 32;
        static constexpr uint32_t MAX_CHUNK_SIZE_KB = 32768;
        static constexpr uint64_t MIN_MEMORY_MB = 256;
        static constexpr uint64_t MAX_MEMORY_MB = 16384;
    };

    class WimlibOptimizerBuilder {
    public:
        WimlibOptimizerBuilder() = default;

        WimlibOptimizerBuilder& WithOptimizationLevel(OptimizationLevel level) {
            mConfig.level = level;
            return *this;
        }

        WimlibOptimizerBuilder& WithMaxThreads(uint32_t threads) {
            mConfig.maxThreads = threads;
            return *this;
        }

        WimlibOptimizerBuilder& WithMemoryLimit(uint64_t memoryMB) {
            mConfig.memoryLimitMB = memoryMB;
            return *this;
        }

        WimlibOptimizerBuilder& WithChunkSize(uint32_t chunkKB) {
            mConfig.chunkSizeKB = chunkKB;
            return *this;
        }

        WimlibOptimizerBuilder& WithSolidCompression(bool enable) {
            mConfig.enableSolidCompression = enable;
            return *this;
        }

        WimlibOptimizerBuilder& WithDeduplication(bool enable) {
            mConfig.enableDeduplication = enable;
            return *this;
        }

        [[nodiscard]] std::unique_ptr<WimlibOptimizer> Build() {
            auto optimizer = std::make_unique<WimlibOptimizer>(mConfig);
            auto initResult = optimizer->Initialize();
            if (!initResult.HasValue()) {
            }
            return optimizer;
        }

    private:
        WimlibOptimizerConfig mConfig;
    };

}
