#include "Win32FileCopyService.h"
#include "adapters/platform/win32/core/Win32HandleFactory.h"
#include "domain/primitives/Error.h"
#include <Windows.h>
#include <algorithm>
#include <thread>
#undef CopyFile
#undef CopyFileW
#undef min
#undef max
#undef GetMessage

namespace winsetup::adapters::platform {

    namespace {
        namespace abs = winsetup::abstractions;
        namespace dom = winsetup::domain;
    }

    Win32FileCopyService::Win32FileCopyService(
        std::shared_ptr<abs::ILogger> logger,
        uint32_t threadCount
    )
        : m_logger(std::move(logger))
        , m_defaultThreadCount(ResolveThreadCount(threadCount))
    {
        if (m_logger)
            m_logger->Info(L"Win32FileCopyService initialized, threads: "
                + std::to_wstring(m_defaultThreadCount));
    }

    Win32FileCopyService::~Win32FileCopyService() = default;

    void Win32FileCopyService::Cancel() noexcept {
        m_cancelled.store(true);
    }

    bool Win32FileCopyService::IsCancelled() const noexcept {
        return m_cancelled.load();
    }

    abs::FileCopyProgress Win32FileCopyService::GetLastProgress() const noexcept {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        return m_lastProgress;
    }

    dom::Expected<void> Win32FileCopyService::CopyFile(
        const std::wstring& srcPath,
        const std::wstring& dstPath,
        const abs::FileCopyOptions& options,
        abs::FileCopyProgressCallback progressCallback
    ) {
        m_cancelled.store(false);

        if (m_logger)
            m_logger->Info(L"CopyFile: " + srcPath + L" -> " + dstPath);

        DWORD srcAttrib = GetFileAttributesW(srcPath.c_str());
        if (srcAttrib == INVALID_FILE_ATTRIBUTES)
            return dom::Error(L"Source file not found: " + srcPath,
                GetLastError(), dom::ErrorCategory::IO);

        if (srcAttrib & FILE_ATTRIBUTE_DIRECTORY)
            return dom::Error(L"Source is a directory, use CopyDirectory: " + srcPath,
                ERROR_INVALID_PARAMETER, dom::ErrorCategory::IO);

        const std::wstring dstParent = dstPath.substr(0, dstPath.find_last_of(L"\\/"));
        auto ensureResult = EnsureDirectory(dstParent);
        if (!ensureResult.HasValue()) return ensureResult;

        auto copyResult = CopySingleFile(srcPath, dstPath,
            options.bufferSizeKB, options.overwrite);
        if (!copyResult.HasValue()) return copyResult;

        if (progressCallback) {
            abs::FileCopyProgress progress;
            progress.totalFiles = 1;
            progress.copiedFiles = 1;
            progress.percentComplete = 100;
            progress.currentFile = srcPath;
            progressCallback(progress);
        }

        return dom::Expected<void>();
    }

    dom::Expected<void> Win32FileCopyService::CopyDirectory(
        const std::wstring& srcDir,
        const std::wstring& dstDir,
        const abs::FileCopyOptions& options,
        abs::FileCopyProgressCallback progressCallback
    ) {
        m_cancelled.store(false);

        if (m_logger)
            m_logger->Info(L"CopyDirectory: " + srcDir + L" -> " + dstDir);

        DWORD srcAttrib = GetFileAttributesW(srcDir.c_str());
        if (srcAttrib == INVALID_FILE_ATTRIBUTES || !(srcAttrib & FILE_ATTRIBUTE_DIRECTORY))
            return dom::Error(L"Source directory not found: " + srcDir,
                ERROR_PATH_NOT_FOUND, dom::ErrorCategory::IO);

        std::vector<CopyTask> tasks;
        auto collectResult = CollectFiles(srcDir, dstDir, options.recursive, tasks);
        if (!collectResult.HasValue()) return collectResult;
        if (tasks.empty()) return dom::Expected<void>();

        uint64_t totalBytes = 0;
        for (const auto& t : tasks) totalBytes += t.fileSize;
        const uint32_t totalFiles = static_cast<uint32_t>(tasks.size());
        const uint32_t threadCount = std::min(
            ResolveThreadCount(options.threadCount), totalFiles);

        std::atomic<uint32_t>   taskIndex{ 0 };
        std::atomic<uint64_t>   copiedBytes{ 0 };
        std::atomic<uint32_t>   copiedFiles{ 0 };
        std::vector<dom::Error> errors;
        std::mutex              errorsMutex;

        auto ctx = std::make_shared<WorkerContext>();
        ctx->service = this;
        ctx->callback = progressCallback;
        ctx->options = options;
        ctx->tasks = &tasks;
        ctx->taskIndex = &taskIndex;
        ctx->copiedBytes = &copiedBytes;
        ctx->copiedFiles = &copiedFiles;
        ctx->errors = &errors;
        ctx->errorsMutex = &errorsMutex;
        ctx->totalBytes = totalBytes;
        ctx->totalFiles = totalFiles;

        std::vector<UniqueHandle> threads;
        threads.reserve(threadCount);
        for (uint32_t i = 0; i < threadCount; ++i) {
            HANDLE hThread = CreateThread(nullptr, 0, WorkerThreadProc, ctx.get(), 0, nullptr);
            if (hThread)
                threads.push_back(Win32HandleFactory::MakeHandle(hThread));
        }

        for (auto& t : threads)
            WaitForSingleObject(Win32HandleFactory::ToWin32Handle(t), INFINITE);

        if (m_cancelled.load())
            return dom::Error(L"Copy operation was cancelled",
                ERROR_OPERATION_ABORTED, dom::ErrorCategory::IO);

        if (!errors.empty()) return errors.front();

        if (m_logger)
            m_logger->Info(L"CopyDirectory completed: "
                + std::to_wstring(copiedFiles.load()) + L" files copied");

        return dom::Expected<void>();
    }

    dom::Expected<void> Win32FileCopyService::CopySingleFile(
        const std::wstring& srcPath,
        const std::wstring& dstPath,
        uint32_t bufferSizeKB,
        bool overwrite
    ) {
        if (!overwrite) {
            if (GetFileAttributesW(dstPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                return dom::Expected<void>();
        }

        const uint32_t bufSize =
            std::clamp(bufferSizeKB, k_minBufferSizeKB, k_maxBufferSizeKB) * 1024;

        auto hSrc = Win32HandleFactory::MakeHandle(
            CreateFileW(srcPath.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN,
                nullptr));
        if (!hSrc)
            return dom::Error(L"Failed to open source file: " + srcPath,
                GetLastError(), dom::ErrorCategory::IO);

        auto hDst = Win32HandleFactory::MakeHandle(
            CreateFileW(dstPath.c_str(),
                GENERIC_WRITE,
                0,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                nullptr));
        if (!hDst)
            return dom::Error(L"Failed to create destination file: " + dstPath,
                GetLastError(), dom::ErrorCategory::IO);

        std::vector<BYTE> buffer(bufSize);
        DWORD bytesRead = 0;
        DWORD bytesWritten = 0;

        while (!m_cancelled.load()) {
            if (!ReadFile(Win32HandleFactory::ToWin32Handle(hSrc),
                buffer.data(), bufSize, &bytesRead, nullptr))
                return dom::Error(L"Read failed: " + srcPath,
                    GetLastError(), dom::ErrorCategory::IO);

            if (bytesRead == 0) break;

            if (!WriteFile(Win32HandleFactory::ToWin32Handle(hDst),
                buffer.data(), bytesRead, &bytesWritten, nullptr)
                || bytesWritten != bytesRead)
                return dom::Error(L"Write failed: " + dstPath,
                    GetLastError(), dom::ErrorCategory::IO);
        }

        DWORD srcAttrib = GetFileAttributesW(srcPath.c_str());
        if (srcAttrib != INVALID_FILE_ATTRIBUTES)
            SetFileAttributesW(dstPath.c_str(), srcAttrib);

        FILETIME ctime{}, atime{}, wtime{};
        if (GetFileTime(Win32HandleFactory::ToWin32Handle(hSrc), &ctime, &atime, &wtime))
            SetFileTime(Win32HandleFactory::ToWin32Handle(hDst), &ctime, &atime, &wtime);

        return dom::Expected<void>();
    }

    dom::Expected<void> Win32FileCopyService::CollectFiles(
        const std::wstring& srcDir,
        const std::wstring& dstDir,
        bool recursive,
        std::vector<CopyTask>& outTasks
    ) {
        auto ensureResult = EnsureDirectory(dstDir);
        if (!ensureResult.HasValue()) return ensureResult;

        WIN32_FIND_DATAW findData{};
        auto hFind = Win32HandleFactory::MakeFindHandle(
            FindFirstFileW((srcDir + L"\\*").c_str(), &findData));
        if (!hFind)
            return dom::Error(L"Failed to enumerate directory: " + srcDir,
                GetLastError(), dom::ErrorCategory::IO);

        do {
            if (m_cancelled.load()) break;
            const std::wstring name = findData.cFileName;
            if (name == L"." || name == L"..") continue;

            const std::wstring src = srcDir + L"\\" + name;
            const std::wstring dst = dstDir + L"\\" + name;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (recursive) {
                    auto sub = CollectFiles(src, dst, recursive, outTasks);
                    if (!sub.HasValue()) return sub;
                }
            }
            else {
                LARGE_INTEGER fileSize{};
                fileSize.HighPart = static_cast<LONG>(findData.nFileSizeHigh);
                fileSize.LowPart = findData.nFileSizeLow;
                outTasks.push_back({ src, dst,
                    static_cast<uint64_t>(fileSize.QuadPart) });
            }
        } while (FindNextFileW(
            Win32HandleFactory::ToWin32FindHandle(hFind), &findData));

        return dom::Expected<void>();
    }

    dom::Expected<void> Win32FileCopyService::EnsureDirectory(
        const std::wstring& dirPath
    ) {
        if (dirPath.empty()) return dom::Expected<void>();

        DWORD attrib = GetFileAttributesW(dirPath.c_str());
        if (attrib != INVALID_FILE_ATTRIBUTES) {
            if (attrib & FILE_ATTRIBUTE_DIRECTORY) return dom::Expected<void>();
            return dom::Error(L"Path exists but is not a directory: " + dirPath,
                ERROR_DIRECTORY, dom::ErrorCategory::IO);
        }

        const size_t pos = dirPath.find_last_of(L"\\/");
        if (pos != std::wstring::npos && pos > 0) {
            auto parent = EnsureDirectory(dirPath.substr(0, pos));
            if (!parent.HasValue()) return parent;
        }

        if (!CreateDirectoryW(dirPath.c_str(), nullptr)) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
                return dom::Error(L"Failed to create directory: " + dirPath,
                    err, dom::ErrorCategory::IO);
        }

        return dom::Expected<void>();
    }

    uint32_t Win32FileCopyService::ResolveThreadCount(uint32_t requested) const noexcept {
        if (requested == 0) {
            uint32_t cores = static_cast<uint32_t>(std::thread::hardware_concurrency());
            if (cores == 0) cores = 4;
            return std::min(cores, k_maxThreadCount);
        }
        return std::clamp(requested, 1u, k_maxThreadCount);
    }

    void Win32FileCopyService::NotifyProgress(
        abs::FileCopyProgressCallback& callback,
        uint64_t copiedBytes,
        uint64_t totalBytes,
        uint32_t copiedFiles,
        uint32_t totalFiles,
        const std::wstring& currentFile
    ) {
        abs::FileCopyProgress progress;
        progress.copiedBytes = copiedBytes;
        progress.totalBytes = totalBytes;
        progress.copiedFiles = copiedFiles;
        progress.totalFiles = totalFiles;
        progress.percentComplete = totalBytes > 0
            ? static_cast<uint32_t>((copiedBytes * 100) / totalBytes) : 0;
        progress.currentFile = currentFile;

        { std::lock_guard<std::mutex> lock(m_progressMutex); m_lastProgress = progress; }
        if (callback) callback(progress);
    }

    unsigned long __stdcall Win32FileCopyService::WorkerThreadProc(void* lpParam) {
        auto* ctx = static_cast<WorkerContext*>(lpParam);
        ctx->service->WorkerRun(ctx);
        return 0;
    }

    void Win32FileCopyService::WorkerRun(WorkerContext* ctx) {
        while (!m_cancelled.load()) {
            const uint32_t idx = ctx->taskIndex->fetch_add(1);
            if (idx >= static_cast<uint32_t>(ctx->tasks->size())) break;

            const auto& task = (*ctx->tasks)[idx];
            auto result = CopySingleFile(
                task.srcPath, task.dstPath,
                ctx->options.bufferSizeKB, ctx->options.overwrite);

            if (!result.HasValue()) {
                if (m_logger)
                    m_logger->Warning(L"Copy failed: " + task.srcPath
                        + L" - " + result.GetError().GetMessage());
                std::lock_guard<std::mutex> lock(*ctx->errorsMutex);
                ctx->errors->push_back(result.GetError());
                continue;
            }

            const uint64_t cb = ctx->copiedBytes->fetch_add(task.fileSize) + task.fileSize;
            const uint32_t cf = ctx->copiedFiles->fetch_add(1) + 1;
            NotifyProgress(ctx->callback, cb, ctx->totalBytes,
                cf, ctx->totalFiles, task.srcPath);
        }
    }

} // namespace winsetup::adapters::platform
