// src/adapters/platform/win32/storage/Win32FileCopyService.h
#pragma once
#include "abstractions/services/storage/IFileCopyService.h"
#include "abstractions/infrastructure/logging/ILogger.h"
#include "adapters/platform/win32/memory/UniqueHandle.h"
#include "adapters/platform/win32/memory/UniqueFindHandle.h"
#include "domain/primitives/Expected.h"
#include "domain/primitives/Error.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>

namespace winsetup::adapters::platform {

    class Win32FileCopyService final : public winsetup::abstractions::IFileCopyService {
    public:
        explicit Win32FileCopyService(
            std::shared_ptr<winsetup::abstractions::ILogger> logger,
            uint32_t threadCount = 0
        );
        ~Win32FileCopyService() override;

        Win32FileCopyService(const Win32FileCopyService&) = delete;
        Win32FileCopyService& operator=(const Win32FileCopyService&) = delete;

        [[nodiscard]] winsetup::domain::Expected<void> CopyFile(
            const std::wstring& srcPath,
            const std::wstring& dstPath,
            const winsetup::abstractions::FileCopyOptions& options,
            winsetup::abstractions::FileCopyProgressCallback progressCallback
        ) override;

        [[nodiscard]] winsetup::domain::Expected<void> CopyDirectory(
            const std::wstring& srcDir,
            const std::wstring& dstDir,
            const winsetup::abstractions::FileCopyOptions& options,
            winsetup::abstractions::FileCopyProgressCallback progressCallback
        ) override;

        void Cancel() noexcept override;
        [[nodiscard]] bool IsCancelled() const noexcept override;
        [[nodiscard]] winsetup::abstractions::FileCopyProgress GetLastProgress() const noexcept override;

    private:
        struct CopyTask {
            std::wstring srcPath;
            std::wstring dstPath;
            uint64_t     fileSize = 0;
        };

        struct WorkerContext {
            Win32FileCopyService* service = nullptr;
            winsetup::abstractions::FileCopyProgressCallback          callback;
            winsetup::abstractions::FileCopyOptions                   options;
            std::vector<CopyTask>* tasks = nullptr;
            std::atomic<uint32_t>* taskIndex = nullptr;
            std::atomic<uint64_t>* copiedBytes = nullptr;
            std::atomic<uint32_t>* copiedFiles = nullptr;
            std::vector<winsetup::domain::Error>* errors = nullptr;
            std::mutex* errorsMutex = nullptr;
            uint64_t                                                  totalBytes = 0;
            uint32_t                                                  totalFiles = 0;
        };

        [[nodiscard]] winsetup::domain::Expected<void> CopySingleFile(
            const std::wstring& srcPath,
            const std::wstring& dstPath,
            uint32_t bufferSizeKB,
            bool overwrite
        );

        [[nodiscard]] winsetup::domain::Expected<void> CollectFiles(
            const std::wstring& srcDir,
            const std::wstring& dstDir,
            bool recursive,
            std::vector<CopyTask>& outTasks
        );

        [[nodiscard]] winsetup::domain::Expected<void> EnsureDirectory(
            const std::wstring& dirPath
        );

        [[nodiscard]] uint32_t ResolveThreadCount(uint32_t requested) const noexcept;

        void NotifyProgress(
            winsetup::abstractions::FileCopyProgressCallback& callback,
            uint64_t copiedBytes,
            uint64_t totalBytes,
            uint32_t copiedFiles,
            uint32_t totalFiles,
            const std::wstring& currentFile
        );

        static unsigned long __stdcall WorkerThreadProc(void* lpParam);
        void WorkerRun(WorkerContext* ctx);

        std::shared_ptr<winsetup::abstractions::ILogger>  m_logger;
        uint32_t                                          m_defaultThreadCount;
        std::atomic<bool>                                 m_cancelled{ false };
        mutable std::mutex                                m_progressMutex;
        winsetup::abstractions::FileCopyProgress          m_lastProgress;

        static constexpr uint32_t k_minBufferSizeKB = 64;
        static constexpr uint32_t k_maxBufferSizeKB = 4096;
        static constexpr uint32_t k_maxThreadCount = 16;
    };

}
