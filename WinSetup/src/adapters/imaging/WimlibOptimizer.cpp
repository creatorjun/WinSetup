// src/adapters/imaging/WimlibOptimizer.cpp
#pragma warning(push)
#pragma warning(disable: 4200)
#include <lib/wimlib.h>
#pragma warning(pop)

#include "WimlibOptimizer.h"
#include <adapters/platform/win32/core/Win32HandleFactory.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <Windows.h>
#include <psapi.h>

#undef min
#undef max

namespace winsetup::adapters {

    WimlibOptimizer::WimlibOptimizer()
        : mConfig()
        , mLastStats()
        , mMemoryPool(nullptr)
        , mMemoryPoolSize(0)
        , mProgressCallback(nullptr)
        , mProgressContext(nullptr)
        , mInitialized(false)
        , mPeakMemory(0)
        , mJobObject()
    {
    }

    WimlibOptimizer::WimlibOptimizer(const WimlibOptimizerConfig& config)
        : mConfig(config)
        , mLastStats()
        , mMemoryPool(nullptr)
        , mMemoryPoolSize(0)
        , mProgressCallback(nullptr)
        , mProgressContext(nullptr)
        , mInitialized(false)
        , mPeakMemory(0)
        , mJobObject()
    {
    }

    WimlibOptimizer::~WimlibOptimizer() {
        ReleaseMemoryPool();
    }

    domain::Expected<void> WimlibOptimizer::Initialize() {
        if (mInitialized.load()) {
            return domain::Expected<void>();
        }

        if (mConfig.maxThreads == 0) {
            mConfig.maxThreads = CalculateOptimalThreadCount();
        }

        if (mConfig.memoryLimitMB == 0) {
            mConfig.memoryLimitMB = CalculateOptimalMemoryLimit();
        }

        auto memResult = AllocateMemoryPool();
        if (!memResult.HasValue()) {
            return memResult;
        }

        auto threadResult = ConfigureThreadPool();
        if (!threadResult.HasValue()) {
            return threadResult;
        }

        HANDLE hJob = CreateJobObjectW(nullptr, nullptr);
        if (hJob) {
            mJobObject = platform::Win32HandleFactory::MakeHandle(hJob);

            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo{};
            jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
            jobInfo.JobMemoryLimit = mConfig.memoryLimitMB * 1024 * 1024;

            SetInformationJobObject(
                platform::Win32HandleFactory::ToWin32Handle(mJobObject),
                JobObjectExtendedLimitInformation,
                &jobInfo,
                sizeof(jobInfo)
            );

            AssignProcessToJobObject(
                platform::Win32HandleFactory::ToWin32Handle(mJobObject),
                GetCurrentProcess()
            );
        }

        mInitialized.store(true);
        return domain::Expected<void>();
    }

    domain::Expected<void> WimlibOptimizer::ApplyImage(
        const std::wstring& wimPath,
        uint32_t imageIndex,
        const std::wstring& targetPath,
        abstractions::ProgressCallback progressCallback)
    {
        return domain::Error{
            L"ApplyImage not implemented yet",
            0,
            domain::ErrorCategory::Imaging
        };
    }

    domain::Expected<void> WimlibOptimizer::CaptureImage(
        const std::wstring& sourcePath,
        const std::wstring& wimPath,
        const std::wstring& imageName,
        const std::wstring& imageDescription,
        abstractions::CompressionType compression,
        abstractions::ProgressCallback progressCallback)
    {
        return domain::Error{
            L"CaptureImage not implemented yet",
            0,
            domain::ErrorCategory::Imaging
        };
    }

    domain::Expected<std::vector<abstractions::ImageInfo>> WimlibOptimizer::GetImageInfo(
        const std::wstring& wimPath)
    {
        return domain::Error{
            L"GetImageInfo not implemented yet",
            0,
            domain::ErrorCategory::Imaging
        };
    }

    domain::Expected<void> WimlibOptimizer::OptimizeImage(
        const std::wstring& wimPath,
        abstractions::CompressionType compression)
    {
        return domain::Error{
            L"OptimizeImage not implemented yet",
            0,
            domain::ErrorCategory::Imaging
        };
    }

    void WimlibOptimizer::SetCompressionLevel(uint32_t level) {
        mConfig.level = static_cast<OptimizationLevel>(level);
    }

    void WimlibOptimizer::SetThreadCount(uint32_t threads) {
        mConfig.maxThreads = threads;
    }

    void WimlibOptimizer::SetMemoryLimit(uint64_t memoryMB) {
        mConfig.memoryLimitMB = memoryMB;
    }

    void WimlibOptimizer::SetConfig(const WimlibOptimizerConfig& config) {
        mConfig = config;
        mInitialized.store(false);
    }

    domain::Expected<void> WimlibOptimizer::OptimizeCapture(
        WIMStruct* wim,
        int compressionType
    ) {
        if (!mInitialized.load()) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult;
            }
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        mLastStats = WimlibOperationStats{};

        auto compResult = ApplyCompressionSettings(wim, compressionType);
        if (!compResult.HasValue()) {
            return compResult;
        }

        if (mProgressCallback) {
            auto callback = reinterpret_cast<wimlib_progress_func_t>(mProgressCallback);
            wimlib_register_progress_function(wim, callback, mProgressContext);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );
        mLastStats.elapsedSeconds = duration.count() / 1000.0;
        mLastStats.peakMemoryMB = mPeakMemory.load() / (1024 * 1024);

        return domain::Expected<void>();
    }

    domain::Expected<void> WimlibOptimizer::OptimizeExtract(WIMStruct* wim) {
        if (!mInitialized.load()) {
            auto initResult = Initialize();
            if (!initResult.HasValue()) {
                return initResult;
            }
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        auto extResult = ApplyExtractionSettings(wim);
        if (!extResult.HasValue()) {
            return extResult;
        }

        if (mProgressCallback) {
            auto callback = reinterpret_cast<wimlib_progress_func_t>(mProgressCallback);
            wimlib_register_progress_function(wim, callback, mProgressContext);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );
        mLastStats.elapsedSeconds = duration.count() / 1000.0;
        mLastStats.peakMemoryMB = mPeakMemory.load() / (1024 * 1024);

        return domain::Expected<void>();
    }

    void WimlibOptimizer::SetProgressCallback(void* callback, void* context) {
        mProgressCallback = callback;
        mProgressContext = context;
    }

    uint32_t WimlibOptimizer::CalculateOptimalThreadCount() const {
        uint32_t coreCount = std::thread::hardware_concurrency();

        if (coreCount == 0) {
            coreCount = 4;
        }

        uint32_t optimalThreads = coreCount;

        switch (mConfig.level) {
        case OptimizationLevel::Fast:
            optimalThreads = (std::min)(coreCount, 4u);
            break;
        case OptimizationLevel::Normal:
            optimalThreads = (std::min)(coreCount, 8u);
            break;
        case OptimizationLevel::Best:
            optimalThreads = (std::min)(coreCount, 12u);
            break;
        case OptimizationLevel::Ultra:
            optimalThreads = coreCount;
            break;
        }

        return optimalThreads;
    }

    uint64_t WimlibOptimizer::CalculateOptimalMemoryLimit() const {
        MEMORYSTATUSEX memStatus{};
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);

        uint64_t totalMemoryMB = memStatus.ullTotalPhys / (1024 * 1024);
        uint64_t availableMemoryMB = memStatus.ullAvailPhys / (1024 * 1024);

        uint64_t optimalMemoryMB = availableMemoryMB / 2;

        switch (mConfig.level) {
        case OptimizationLevel::Fast:
            optimalMemoryMB = (std::min)(optimalMemoryMB, 1024ULL);
            break;
        case OptimizationLevel::Normal:
            optimalMemoryMB = (std::min)(optimalMemoryMB, 2048ULL);
            break;
        case OptimizationLevel::Best:
            optimalMemoryMB = (std::min)(optimalMemoryMB, 4096ULL);
            break;
        case OptimizationLevel::Ultra:
            optimalMemoryMB = (std::min)(optimalMemoryMB, 8192ULL);
            break;
        }

        optimalMemoryMB = (std::max)(optimalMemoryMB, MIN_MEMORY_MB);
        optimalMemoryMB = (std::min)(optimalMemoryMB, MAX_MEMORY_MB);

        return optimalMemoryMB;
    }

    uint32_t WimlibOptimizer::CalculateOptimalChunkSize(
        uint64_t estimatedSizeBytes
    ) const {
        uint32_t chunkSizeKB = mConfig.chunkSizeKB;

        if (chunkSizeKB == 0) {
            if (estimatedSizeBytes < 100ULL * 1024 * 1024) {
                chunkSizeKB = 32;
            }
            else if (estimatedSizeBytes < 1ULL * 1024 * 1024 * 1024) {
                chunkSizeKB = 64;
            }
            else if (estimatedSizeBytes < 10ULL * 1024 * 1024 * 1024) {
                chunkSizeKB = 128;
            }
            else {
                chunkSizeKB = 256;
            }

            if (mConfig.enableSolidCompression) {
                chunkSizeKB *= 2;
            }
        }

        chunkSizeKB = (std::max)(chunkSizeKB, MIN_CHUNK_SIZE_KB);
        chunkSizeKB = (std::min)(chunkSizeKB, MAX_CHUNK_SIZE_KB);

        uint32_t powerOf2 = 1;
        while (powerOf2 < chunkSizeKB) {
            powerOf2 *= 2;
        }
        chunkSizeKB = powerOf2;

        return chunkSizeKB;
    }

    domain::Expected<void> WimlibOptimizer::AllocateMemoryPool() {
        if (mMemoryPool) {
            ReleaseMemoryPool();
        }

        mMemoryPoolSize = mConfig.memoryLimitMB * 1024 * 1024;

        mMemoryPool = VirtualAlloc(
            nullptr,
            mMemoryPoolSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        if (!mMemoryPool) {
            return domain::Error{
                L"Failed to allocate memory pool",
                GetLastError(),
                domain::ErrorCategory::System
            };
        }

        return domain::Expected<void>();
    }

    void WimlibOptimizer::ReleaseMemoryPool() {
        if (mMemoryPool) {
            VirtualFree(mMemoryPool, 0, MEM_RELEASE);
            mMemoryPool = nullptr;
            mMemoryPoolSize = 0;
        }
    }

    domain::Expected<void> WimlibOptimizer::ConfigureThreadPool() {
        wimlib_set_default_compression_level(
            static_cast<int>(WIMLIB_COMPRESSION_TYPE_LZX),
            static_cast<uint32_t>(mConfig.level)
        );

        return domain::Expected<void>();
    }

    domain::Expected<void> WimlibOptimizer::ApplyCompressionSettings(
        WIMStruct* wim,
        int compressionType
    ) {
        int ret = wimlib_set_output_compression_type(
            wim,
            static_cast<enum wimlib_compression_type>(compressionType)
        );

        if (ret != 0) {
            return domain::Error{
                L"Failed to set compression type",
                static_cast<uint32_t>(ret),
                domain::ErrorCategory::Imaging
            };
        }

        uint32_t chunkSize = mConfig.chunkSizeKB * 1024;
        ret = wimlib_set_output_chunk_size(wim, chunkSize);
        if (ret != 0) {
            return domain::Error{
                L"Failed to set chunk size",
                static_cast<uint32_t>(ret),
                domain::ErrorCategory::Imaging
            };
        }

        return domain::Expected<void>();
    }

    domain::Expected<void> WimlibOptimizer::ApplyExtractionSettings(
        WIMStruct* wim
    ) {
        return domain::Expected<void>();
    }

    void WimlibOptimizer::UpdateStats(const void* info) {
        uint64_t currentMemory = GetCurrentMemoryUsage();

        uint64_t expected = mPeakMemory.load();
        while (currentMemory > expected &&
            !mPeakMemory.compare_exchange_weak(expected, currentMemory)) {
        }
    }

    uint64_t WimlibOptimizer::GetCurrentMemoryUsage() const {
        PROCESS_MEMORY_COUNTERS_EX pmc{};
        pmc.cb = sizeof(pmc);

        if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
            sizeof(pmc)
        )) {
            return pmc.WorkingSetSize;
        }

        return 0;
    }

}
