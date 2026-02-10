// src/adapters/imaging/WimlibOptimizer.h
#pragma once

#include "../../lib/wimlib.h"
#include "../../domain/primitives/Expected.h"
#include <cstdint>
#include <memory>
#include <atomic>
#include <Windows.h>

namespace winsetup::adapters {

    enum class OptimizationLevel {
        Fast = 1,           // 빠른 압축 (낮은 압축률)
        Normal = 6,         // 기본 균형
        Best = 12,          // 최고 압축률 (느림)
        Ultra = 20          // 초고압축 (매우 느림)
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
            , maxThreads(0)  // 0 = 자동 (CPU 코어 수)
            , memoryLimitMB(2048)  // 2GB
            , chunkSizeKB(32)  // 32KB
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

    class WimlibOptimizer {
    public:
        WimlibOptimizer();
        explicit WimlibOptimizer(const WimlibOptimizerConfig& config);
        ~WimlibOptimizer();

        WimlibOptimizer(const WimlibOptimizer&) = delete;
        WimlibOptimizer& operator=(const WimlibOptimizer&) = delete;

        [[nodiscard]] domain::Expected<void> Initialize();

        void SetConfig(const WimlibOptimizerConfig& config);

        [[nodiscard]] const WimlibOptimizerConfig& GetConfig() const noexcept {
            return mConfig;
        }

        [[nodiscard]] domain::Expected<void> OptimizeCapture(
            WIMStruct* wim,
            int compressionType = WIMLIB_COMPRESSION_TYPE_LZX
        );

        [[nodiscard]] domain::Expected<void> OptimizeExtract(
            WIMStruct* wim
        );

        [[nodiscard]] const WimlibOperationStats& GetLastStats() const noexcept {
            return mLastStats;
        }

        void SetProgressCallback(
            wimlib_progress_func_t callback,
            void* context
        );

        [[nodiscard]] uint32_t CalculateOptimalThreadCount() const;

        [[nodiscard]] uint64_t CalculateOptimalMemoryLimit() const;

        [[nodiscard]] uint32_t CalculateOptimalChunkSize(
            uint64_t estimatedSizeBytes
        ) const;

    private:
        [[nodiscard]] domain::Expected<void> AllocateMemoryPool();

        void ReleaseMemoryPool();

        [[nodiscard]] domain::Expected<void> ConfigureThreadPool();

        [[nodiscard]] domain::Expected<void> ApplyCompressionSettings(
            WIMStruct* wim,
            int compressionType
        );

        [[nodiscard]] domain::Expected<void> ApplyExtractionSettings(
            WIMStruct* wim
        );

        void UpdateStats(const wimlib_progress_info& info);

        [[nodiscard]] uint64_t GetCurrentMemoryUsage() const;

        WimlibOptimizerConfig mConfig;
        WimlibOperationStats mLastStats;

        void* mMemoryPool;
        size_t mMemoryPoolSize;

        wimlib_progress_func_t mProgressCallback;
        void* mProgressContext;

        std::atomic<bool> mInitialized;
        std::atomic<uint64_t> mPeakMemory;

        HANDLE mJobObject;

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
