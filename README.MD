# WinSetup 클린 아키텍처 최종 구현 계획서 v1.0

## 🎯 개요

### 목표
Windows PE 환경에서 실행되는 완벽한 PC 초기화 프로그램을 **클린 아키텍처 + 저수준 최적화 + 프로덕션 안정성**을 모두 충족하여 구현합니다.

### 핵심 설계 원칙
1. **완전한 계층 격리**: Domain은 외부 의존성 0 (Windows.h 절대 금지)
2. **타입 안전성**: void* 금지, 컴파일 타임 타입 체크
3. **RAII 강제**: 모든 리소스는 자동 정리
4. **트랜잭션**: 원자성 보장, 실패 시 롤백
5. **고성능**: IOCTL, MFT, wimlib 최적화
6. **테스트 가능성**: 모든 비즈니스 로직 단위 테스트

### 기술 스택
- **언어**: C++20/23 (Concepts, Coroutines, Ranges)
- **플랫폼**: Windows PE (Windows 10/11)
- **빌드**: Visual Studio 2022 Solution (.sln + .vcxproj)
- **저수준 API**:
  - IOCTL (디스크 제어)
  - FSCTL (MFT 직접 읽기)
  - GetSystemFirmwareTable (SMBIOS)
  - OVERLAPPED (진정한 비동기 I/O)
  - wimlib (이미지 처리)
  - DismApi (드라이버 주입)
- **테스트**: Google Test + Google Mock

### 예상 성능 지표
- **디스크 열거**: 10개 디스크 < 3초 (병렬 IOCTL)
- **파티션 분석**: 10개 볼륨 < 1초 (MFT 직접 읽기)
- **이미지 적용**: Config.ini 예상 시간 준수
- **메모리 사용**: WinPE 환경 < 512MB

---

## 📐 아키텍처 원칙

### 1. 절대 의존성 규칙 (Absolute Dependency Rule)

```
┌─────────────────────────────────────────┐
│  Domain (Layer 1)                       │
│  ✓ 외부 의존성 0                         │
│  ✓ 표준 C++만 사용                       │
│  ✓ Windows.h 절대 금지                  │
│  ✓ Expected, Error 등 기본 제공          │
│  ✗ HANDLE, HWND 등 플랫폼 타입 금지      │
└─────────────────────────────────────────┘
            ↑
            │ (Domain 타입만 참조)
┌─────────────────────────────────────────┐
│  Abstractions (Layer 0)                 │
│  ✓ 순수 인터페이스                       │
│  ✓ Domain 타입 사용                      │
│  ✗ 구현 코드 금지                        │
└─────────────────────────────────────────┘
            ↑
            │ (인터페이스만 의존)
┌─────────────────────────────────────────┐
│  Application (Layer 2)                  │
│  ✓ Use Cases                            │
│  ✓ Task<T> 코루틴                       │
│  ✓ DIContainer                          │
│  ✗ 플랫폼 코드 금지                      │
└─────────────────────────────────────────┘
            ↑
            │ (인터페이스 구현)
┌─────────────────────────────────────────┐
│  Adapters (Layer 3)                     │
│  ✓ IOCTL + OVERLAPPED                   │
│  ✓ MFT 직접 읽기                         │
│  ✓ SMBIOS 파싱                          │
│  ✓ wimlib 최적화                        │
│  ✓ DiskTransaction                      │
│  ✓ Win32 RAII 래퍼 (UniqueHandle 등)    │
└─────────────────────────────────────────┘
            ↑
            │ (모든 것 연결)
┌─────────────────────────────────────────┐
│  Main (Layer 4)                         │
│  ✓ Composition Root                     │
│  ✓ Service Registration                 │
└─────────────────────────────────────────┘
```

### 2. RAII 강제 규칙

```cpp
// ❌ 절대 금지: 수동 리소스 관리
void BadExample() {
    HANDLE hDisk = CreateFile(...);
    // ... 중간에 return하면 누수!
    CloseHandle(hDisk);
}

// ✅ 필수: RAII 래퍼 사용 (Adapters 계층에 위치)
void GoodExample() {
    auto hDisk = adapters::win32::UniqueHandle(CreateFile(...));
    // 자동으로 CloseHandle 호출
}

// ✅ 모든 Win32 리소스에 적용
adapters::win32::UniqueHandle hFile;
adapters::win32::UniqueLibrary hModule;
adapters::win32::UniqueFindHandle hFind;
```

### 3. 에러 처리 규칙

```cpp
// ❌ 금지: 예외 던지기 (WinPE 환경에서 위험)
DiskInfo GetDiskInfo(uint32_t index) {
    if (error) throw std::runtime_error("Failed");
}

// ❌ 금지: 에러 코드 반환 (체크 안 할 수 있음)
int GetDiskInfo(uint32_t index, DiskInfo* out) {
    if (error) return -1;
}

// ✅ 필수: Expected<T> 사용
Expected<DiskInfo> GetDiskInfo(uint32_t index) noexcept {
    if (error) return Error{L"Failed", GetLastError()};
    return DiskInfo{...};
}

// ✅ Monadic 체이닝
auto result = GetDiskInfo(0)
    .Map([](const auto& disk) { return disk.GetSize(); })# WinSetup 클린 아키텍처 최종 구현 계획서 v1.0

## 🎮 Application 계층 설계

### DIContainer - 의존성 주입

#### application/core/DIContainer.h
```cpp
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <any>
#include <mutex>

namespace winsetup::application {

    enum class ServiceLifetime {
        Singleton,
        Transient,
        Scoped
    };

    class DIContainer {
    public:
        template<typename TInterface, typename TImplementation>
        void Register(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>();
            };

            mRegistrations[typeid(TInterface)] = {factory, lifetime};
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>(
                    Resolve<TDeps>()...
                );
            };

            mRegistrations[typeid(TInterface)] = {factory, lifetime};
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::lock_guard lock(mMutex);

            mSingletons[typeid(TInterface)] = instance;

            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
            };

            mRegistrations[typeid(TInterface)] = {factory, ServiceLifetime::Singleton};
        }

        template<typename TInterface>
        std::shared_ptr<TInterface> Resolve() {
            std::lock_guard lock(mMutex);

            auto typeIndex = typeid(TInterface);

            if (auto it = mRegistrations.find(typeIndex); it != mRegistrations.end()) {
                const auto& [factory, lifetime] = it->second;

                if (lifetime == ServiceLifetime::Singleton) {
                    if (auto singletonIt = mSingletons.find(typeIndex); 
                        singletonIt != mSingletons.end()) {
                        return std::static_pointer_cast<TInterface>(singletonIt->second);
                    }

                    auto instance = factory();
                    mSingletons[typeIndex] = instance;
                    return std::static_pointer_cast<TInterface>(instance);
                }

                return std::static_pointer_cast<TInterface>(factory());
            }

            return nullptr;
        }

        void Clear() {
            std::lock_guard lock(mMutex);
            mSingletons.clear();
            mRegistrations.clear();
        }

    private:
        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime;
        };

        std::unordered_map<std::type_index, Registration> mRegistrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;
        std::mutex mMutex;
    };

}
```

### Task<T> - 코루틴 구현

#### application/async/Task.h
```cpp
#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include "../../domain/primitives/Expected.h"

namespace winsetup::application {

    template<typename T>
    class Task {
    public:
        struct promise_type {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_value(T value) {
                mResult = domain::Expected<T>(std::move(value));
            }

            void unhandled_exception() {
                mException = std::current_exception();
            }

            domain::Expected<T> mResult{domain::Error{L"Not completed", 0}};
            std::exception_ptr mException;
        };

        Task(std::coroutine_handle<promise_type> handle)
            : mHandle(handle)
        {}

        ~Task() {
            if (mHandle) {
                mHandle.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept
            : mHandle(std::exchange(other.mHandle, {}))
        {}

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (mHandle) {
                    mHandle.destroy();
                }
                mHandle = std::exchange(other.mHandle, {});
            }
            return *this;
        }

        void Resume() {
            if (mHandle && !mHandle.done()) {
                mHandle.resume();
            }
        }

        bool IsDone() const {
            return !mHandle || mHandle.done();
        }

        domain::Expected<T> GetResult() {
            if (!IsDone()) {
                return domain::Error{L"Task not completed", 0};
            }

            if (mHandle.promise().mException) {
                std::rethrow_exception(mHandle.promise().mException);
            }

            return std::move(mHandle.promise().mResult);
        }

    private:
        std::coroutine_handle<promise_type> mHandle;
    };

    template<>
    class Task<void> {
    public:
        struct promise_type {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_void() {}

            void unhandled_exception() {
                mException = std::current_exception();
            }

            std::exception_ptr mException;
        };

        Task(std::coroutine_handle<promise_type> handle)
            : mHandle(handle)
        {}

        ~Task() {
            if (mHandle) {
                mHandle.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept
            : mHandle(std::exchange(other.mHandle, {}))
        {}

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (mHandle) {
                    mHandle.destroy();
                }
                mHandle = std::exchange(other.mHandle, {});
            }
            return *this;
        }

        void Resume() {
            if (mHandle && !mHandle.done()) {
                mHandle.resume();
            }
        }

        bool IsDone() const {
            return !mHandle || mHandle.done();
        }

    private:
        std::coroutine_handle<promise_type> mHandle;
    };

}
```

### InstallWindowsUseCase - 핵심 유즈케이스

#### application/usecases/install/InstallWindowsUseCase.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/SetupConfig.h"
#include "../../../domain/entities/DiskInfo.h"
#include "../../../abstractions/services/storage/IDiskService.h"
#include "../../../abstractions/services/storage/IImagingService.h"
#include "../../../abstractions/infrastructure/logging/ILogger.h"
#include "../../../abstractions/infrastructure/messaging/IEventBus.h"
#include "../../async/Task.h"
#include <memory>

namespace winsetup::application {

    class InstallWindowsUseCase {
    public:
        InstallWindowsUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IImagingService> imagingService,
            std::shared_ptr<abstractions::ILogger> logger,
            std::shared_ptr<abstractions::IEventBus> eventBus
        );

        [[nodiscard]] Task<domain::Expected<void>> Execute(
            const domain::SetupConfig& config,
            const domain::DiskInfo& targetDisk
        );

    private:
        [[nodiscard]] domain::Expected<void> PrepareTargetDisk(
            uint32_t diskIndex,
            bool isUEFI
        );

        [[nodiscard]] domain::Expected<void> ApplyWindowsImage(
            const std::wstring& wimPath,
            uint32_t imageIndex,
            wchar_t targetDrive
        );

        [[nodiscard]] domain::Expected<void> ConfigureBootLoader(
            wchar_t systemDrive,
            wchar_t efiDrive,
            bool isUEFI
        );

        void PublishProgress(
            const std::wstring& stage,
            uint32_t percentage
        );

        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::IImagingService> mImagingService;
        std::shared_ptr<abstractions::ILogger> mLogger;
        std::shared_ptr<abstractions::IEventBus> mEventBus;
    };

}
```

---

## 🔌 Adapters 계층 설계 (저수준 구현)

### UniqueHandle - Win32 RAII 래퍼

#### adapters/platform/win32/memory/UniqueHandle.h
```cpp
#pragma once

#include <Windows.h>
#include <utility>

namespace winsetup::adapters::win32 {

    class UniqueHandle {
    public:
        explicit UniqueHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept
            : mHandle(handle)
        {}

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : mHandle(std::exchange(other.mHandle, INVALID_HANDLE_VALUE))
        {}

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                mHandle = std::exchange(other.mHandle, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return mHandle;
        }

        [[nodiscard]] HANDLE Release() noexcept {
            return std::exchange(mHandle, INVALID_HANDLE_VALUE);
        }

        void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
            Close();
            mHandle = handle;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHandle != INVALID_HANDLE_VALUE && mHandle != nullptr;
        }

        [[nodiscard]] HANDLE* AddressOf() noexcept {
            return &mHandle;
        }

    private:
        void Close() noexcept {
            if (mHandle != INVALID_HANDLE_VALUE && mHandle != nullptr) {
                CloseHandle(mHandle);
                mHandle = INVALID_HANDLE_VALUE;
            }
        }

        HANDLE mHandle;
    };

    class UniqueLibrary {
    public:
        explicit UniqueLibrary(HMODULE hModule = nullptr) noexcept
            : mHModule(hModule)
        {}

        ~UniqueLibrary() noexcept {
            if (mHModule) {
                FreeLibrary(mHModule);
            }
        }

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : mHModule(std::exchange(other.mHModule, nullptr))
        {}

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                if (mHModule) {
                    FreeLibrary(mHModule);
                }
                mHModule = std::exchange(other.mHModule, nullptr);
            }
            return *this;
        }

        [[nodiscard]] HMODULE Get() const noexcept {
            return mHModule;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHModule != nullptr;
        }

    private:
        HMODULE mHModule;
    };

    class UniqueFindHandle {
    public:
        explicit UniqueFindHandle(HANDLE hFind = INVALID_HANDLE_VALUE) noexcept
            : mHFind(hFind)
        {}

        ~UniqueFindHandle() noexcept {
            if (mHFind != INVALID_HANDLE_VALUE) {
                FindClose(mHFind);
            }
        }

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : mHFind(std::exchange(other.mHFind, INVALID_HANDLE_VALUE))
        {}

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                if (mHFind != INVALID_HANDLE_VALUE) {
                    FindClose(mHFind);
                }
                mHFind = std::exchange(other.mHFind, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return mHFind;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHFind != INVALID_HANDLE_VALUE;
        }

    private:
        HANDLE mHFind;
    };

}
```

### Win32StringHelper - 문자열 유틸리티

#### adapters/platform/win32/core/Win32StringHelper.h
```cpp
#pragma once

#include <string>
#include <Windows.h>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    class Win32StringHelper {
    public:
        [[nodiscard]] static domain::Expected<std::wstring> UTF8ToWide(
            const std::string& utf8
        );

        [[nodiscard]] static domain::Expected<std::string> WideToUTF8(
            const std::wstring& wide
        );

        [[nodiscard]] static std::wstring FormatErrorMessage(DWORD errorCode);

        [[nodiscard]] static std::wstring NormalizePath(
            const std::wstring& path
        );

        [[nodiscard]] static std::wstring GetVolumePath(
            const std::wstring& path
        );

        [[nodiscard]] static bool IsAbsolutePath(const std::wstring& path);

        static std::wstring ToUpper(const std::wstring& str);
        static std::wstring ToLower(const std::wstring& str);
    };

}
```

### Win32HandleFactory - 핸들 팩토리

#### adapters/platform/win32/core/Win32HandleFactory.h
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>
#include "../../../../domain/primitives/Expected.h"
#include "../memory/UniqueHandle.h"

namespace winsetup::adapters::win32 {

    class Win32HandleFactory {
    public:
        [[nodiscard]] static domain::Expected<UniqueHandle> OpenDisk(
            uint32_t diskIndex,
            DWORD accessFlags = GENERIC_READ | GENERIC_WRITE
        );

        [[nodiscard]] static domain::Expected<UniqueHandle> OpenVolume(
            const std::wstring& volumePath,
            DWORD accessFlags = GENERIC_READ
        );

        [[nodiscard]] static domain::Expected<UniqueHandle> OpenFile(
            const std::wstring& filePath,
            DWORD desiredAccess,
            DWORD shareMode,
            DWORD creationDisposition
        );

    private:
        static std::wstring BuildDiskPath(uint32_t diskIndex);
        static std::wstring BuildVolumePath(const std::wstring& volumePath);
    };

}
```

### DiskTransaction - Step 기반 트랜잭션

#### adapters/platform/win32/storage/DiskTransaction.h
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>
#include "../../../../domain/primitives/Expected.h"
#include "../../../../abstractions/services/storage/IDiskService.h"
#include "../../../../abstractions/infrastructure/logging/ILogger.h"

namespace winsetup::adapters::win32 {

    class DiskTransaction {
    public:
        DiskTransaction(
            uint32_t diskIndex,
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<void> Execute(
            std::function<domain::Expected<void>()> operation
        );

        void AddStep(
            const std::wstring& description,
            std::function<domain::Expected<void>()> execute,
            std::function<domain::Expected<void>()> rollback
        );

        void AddCleanDiskStep();
        
        void AddCreatePartitionLayoutStep(
            const abstractions::PartitionLayout& layout
        );

        void AddFormatPartitionStep(
            uint32_t partitionIndex,
            domain::FileSystemType fileSystem,
            bool quickFormat = true
        );

        [[nodiscard]] domain::Expected<void> ExecuteSteps();

        void SetTimeout(uint32_t timeoutMs) { mTimeoutMs = timeoutMs; }
        void SetAutoRollback(bool enabled) { mAutoRollback = enabled; }

    private:
        struct TransactionStep {
            std::wstring description;
            std::function<domain::Expected<void>()> execute;
            std::function<domain::Expected<void>()> rollback;
            bool executed;
            std::chrono::system_clock::time_point timestamp;

            TransactionStep(
                std::wstring desc,
                std::function<domain::Expected<void>()> exec,
                std::function<domain::Expected<void>()> roll
            )
                : description(std::move(desc))
                , execute(std::move(exec))
                , rollback(std::move(roll))
                , executed(false)
            {}
        };

        [[nodiscard]] domain::Expected<void> BackupCurrentLayout();
        [[nodiscard]] domain::Expected<void> RestoreBackupLayout();
        [[nodiscard]] domain::Expected<void> RollbackSteps();

        void LogStep(const std::wstring& message);
        bool CheckTimeout() const;

        uint32_t mDiskIndex;
        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::ILogger> mLogger;

        std::vector<TransactionStep> mSteps;
        std::optional<abstractions::PartitionLayout> mBackupLayout;
        bool mLayoutBackedUp;

        uint32_t mTimeoutMs;
        bool mAutoRollback;
        std::chrono::system_clock::time_point mStartTime;
        std::vector<std::wstring> mTransactionLog;
    };

}
```

### Win32Logger - 버퍼링 로거

#### adapters/platform/win32/logging/Win32Logger.h
```cpp
#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <source_location>
#include "../../../../abstractions/infrastructure/logging/ILogger.h"
#include "../../../../abstractions/infrastructure/logging/LogLevel.h"

namespace winsetup::adapters::win32 {

    class Win32Logger : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath);
        ~Win32Logger() override;

        void Log(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) override;

        void Flush() override;

        void SetMinimumLevel(abstractions::LogLevel level) {
            mMinLevel = level;
        }

        void SetBufferSize(size_t size) {
            mBufferSize = size;
        }

    private:
        void FlushBuffer();
        void FlushIfNeeded(abstractions::LogLevel level);
        
        std::wstring FormatLogEntry(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location
        );

        std::wstring LevelToString(abstractions::LogLevel level);

        static constexpr size_t DEFAULT_BUFFER_SIZE = 100;

        std::wstring mLogFilePath;
        std::wofstream mFileStream;
        std::vector<std::wstring> mBuffer;
        size_t mBufferSize;
        abstractions::LogLevel mMinLevel;
        std::mutex mMutex;
    };

}
```

### MFTScanner - 고속 파일 스캔

#### adapters/platform/win32/storage/MFTScanner.h
```cpp
#pragma once

#include <Windows.h>
#include <winioctl.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    struct VolumeContents {
        bool hasWindowsDirectory;
        bool hasSystem32Directory;
        bool hasUsersDirectory;
        bool hasProgramFilesDirectory;
        std::unordered_set<std::wstring> topLevelDirectories;
        uint64_t totalFiles;
        uint64_t totalDirectories;
    };

    class MFTScanner {
    public:
        [[nodiscard]] static domain::Expected<VolumeContents> ScanVolume(
            const std::wstring& volumePath
        );

    private:
        struct MFTRecord {
            uint64_t fileReference;
            uint64_t parentFileReference;
            std::wstring fileName;
            bool isDirectory;
        };

        [[nodiscard]] static domain::Expected<std::vector<MFTRecord>> ReadMFT(
            HANDLE hVolume
        );

        static VolumeContents AnalyzeMFTRecords(
            const std::vector<MFTRecord>& records
        );

        static constexpr size_t BUFFER_SIZE = 64 * 1024;
        static constexpr uint32_t MFT_RECORD_SIZE = 1024;
    };

}
```

---

## 🏗️ Visual Studio 프로젝트 구성

### 솔루션 구조

```
WinSetup.sln
├── WinSetup (실행 프로젝트)
│   ├── 플랫폼: x64
│   ├── 구성: Debug, Release
│   ├── 출력 형식: Application (.exe)
│   ├── C++ 표준: C++20 (/std:c++20)
│   ├── 문자 집합: Unicode
│   └── 링크 라이브러리:
│       ├── wimlib.lib
│       ├── DismApi.lib
│       ├── setupapi.lib
│       └── ntdll.lib
│
└── WinSetup.Tests (테스트 프로젝트)
    ├── 플랫폼: x64
    ├── 구성: Debug, Release
    ├── 출력 형식: Application (.exe)
    ├── 링크 라이브러리:
    │   ├── gtest.lib
    │   ├── gtest_main.lib
    │   └── gmock.lib
    └── 테스트 프레임워크: Google Test
```

### WinSetup.vcxproj 주요 설정

```xml
<PropertyGroup>
  <ConfigurationType>Application</ConfigurationType>
  <PlatformToolset>v143</PlatformToolset>
  <CharacterSet>Unicode</CharacterSet>
  <LanguageStandard>stdcpp20</LanguageStandard>
</PropertyGroup>

<ItemDefinitionGroup>
  <ClCompile>
    <WarningLevel>Level4</WarningLevel>
    <TreatWarningAsError>true</TreatWarningAsError>
    <ConformanceMode>true</ConformanceMode>
    <MultiProcessorCompilation>true</MultiProcessorCompilation>
    <PreprocessorDefinitions>
      _UNICODE;
      UNICODE;
      WIN32_LEAN_AND_MEAN;
      NOMINMAX;
      _WIN32_WINNT=0x0A00;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <AdditionalIncludeDirectories>
      $(ProjectDir)src;
      $(ProjectDir)lib;
      %(AdditionalIncludeDirectories)
    </AdditionalIncludeDirectories>
  </ClCompile>
  
  <Link>
    <SubSystem>Windows</SubSystem>
    <AdditionalLibraryDirectories>
      $(ProjectDir)lib;
      %(AdditionalLibraryDirectories)
    </AdditionalLibraryDirectories>
    <AdditionalDependencies>
      wimlib.lib;
      DismApi.lib;
      setupapi.lib;
      ntdll.lib;
      %(AdditionalDependencies)
    </AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

### 빌드 구성

#### Debug 구성
```xml
<PropertyGroup Condition="'$(Configuration)'=='Debug'">
  <LinkIncremental>true</LinkIncremental>
  <GenerateDebugInformation>true</GenerateDebugInformation>
</PropertyGroup>

<ItemDefinitionGroup Condition="'$(Configuration)'=='Debug'">
  <ClCompile>
    <Optimization>Disabled</Optimization>
    <PreprocessorDefinitions>
      _DEBUG;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
  </ClCompile>
</ItemDefinitionGroup>
```

#### Release 구성
```xml
<PropertyGroup Condition="'$(Configuration)'=='Release'">
  <LinkIncremental>false</LinkIncremental>
  <GenerateDebugInformation>false</GenerateDebugInformation>
</PropertyGroup>

<ItemDefinitionGroup Condition="'$(Configuration)'=='Release'">
  <ClCompile>
    <Optimization>MaxSpeed</Optimization>
    <FunctionLevelLinking>true</FunctionLevelLinking>
    <IntrinsicFunctions>true</IntrinsicFunctions>
    <PreprocessorDefinitions>
      NDEBUG;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
  </ClCompile>
  
  <Link>
    <EnableCOMDATFolding>true</EnableCOMDATFolding>
    <OptimizeReferences>true</OptimizeReferences>
  </Link>
</ItemDefinitionGroup>
```

### 필터 구조 (.vcxproj.filters)

```xml
<ItemGroup>
  <Filter Include="src">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\domain">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\domain\primitives">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\abstractions">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\application">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\adapters">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\adapters\platform\win32">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\main">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
</ItemGroup>
```

---

## 📅 구현 단계 (Phase별)

### Phase 1: 기초 인프라 (1주)

**목표**: 완전히 독립적인 기본 타입 구현

**구현 순서**:
1. **domain/primitives/Error** (1일)
   - ErrorCategory enum
   - Error 클래스
   - ErrorContext 구조체
   - ToString() 구현

2. **domain/primitives/Expected** (2일)
   - Expected<T> 템플릿
   - Expected<void> 특수화
   - Monadic 연산 (Map, FlatMap, UnwrapOr)
   - 테스트 작성

3. **domain/memory/PoolAllocator** (1일)
   - 메모리 풀 구현
   - 스레드 안전성 보장
   - 테스트 작성

4. **abstractions/infrastructure/logging** (2일)
   - LogLevel enum
   - ILogger 인터페이스
   - 기본 로깅 구조 정의

**완료 기준**:
- ✅ Expected<T> 단위 테스트 통과
- ✅ PoolAllocator 스레드 안전성 테스트 통과
- ✅ 모든 코드 컴파일 성공

### Phase 2: Domain Entities (1주)

**목표**: 비즈니스 엔티티 및 값 객체 구현

**구현 순서**:
1. **domain/valueobjects** (2일)
   - BusType, DiskType enums
   - DiskSize 클래스
   - FileSystemType enum
   - PartitionType enum
   - DriveLetter 클래스

2. **domain/entities** (3일)
   - DiskInfo
   - PartitionInfo
   - VolumeInfo
   - SystemInfo
   - SetupConfig

3. **domain/services** (2일)
   - DiskSortingService
   - PartitionAnalyzer
   - PathNormalizer

**완료 기준**:
- ✅ 모든 엔티티 불변성 보장
- ✅ 엔티티 단위 테스트 작성
- ✅ 도메인 서비스 로직 검증

### Phase 3: Win32 Core Infrastructure (1주)

**목표**: Win32 플랫폼 추상화 계층

**구현 순서**:
1. **adapters/platform/win32/memory** (1일)
   - UniqueHandle
   - UniqueLibrary
   - UniqueFindHandle

2. **adapters/platform/win32/core** (2일)
   - Win32TypeMapper
   - Win32ErrorHandler
   - Win32StringHelper
   - Win32HandleFactory
   - Win32Constants

3. **adapters/platform/win32/logging** (2일)
   - Win32Logger 구현
   - 버퍼링 전략
   - Flush 정책
   - 파일 로테이션

4. **통합 테스트** (2일)
   - RAII 래퍼 안전성 검증
   - 로거 성능 테스트
   - 메모리 누수 검사

**완료 기준**:
- ✅ UniqueHandle 리소스 자동 해제 검증
- ✅ Win32Logger 버퍼링 동작 확인
- ✅ 메모리 누수 없음

### Phase 4: Application Layer (1주)

**목표**: Use Cases 및 DI Container

**구현 순서**:
1. **application/core** (2일)
   - DIContainer 구현
   - ServiceLocator
   - 의존성 주입 테스트

2. **application/async** (1일)
   - Task<T> 코루틴
   - CancellationToken
   - Promise/Awaitable

3. **application/usecases/system** (2일)
   - AnalyzeSystemUseCase
   - LoadConfigurationUseCase

4. **application/services** (2일)
   - EventBus
   - Dispatcher
   - TaskScheduler

**완료 기준**:
- ✅ DIContainer 순환 의존성 검증
- ✅ 코루틴 동작 테스트
- ✅ EventBus 동시성 테스트

### Phase 5: Storage Adapters (2주)

**목표**: 디스크/볼륨 서비스 구현

**구현 순서**:
1. **IOCTLWrapper + AsyncIOCTL** (3일)
   - IOCTL_DISK_* 래핑
   - OVERLAPPED 비동기 I/O
   - 에러 처리

2. **Win32DiskService** (3일)
   - EnumerateDisks
   - GetDiskInfo
   - CleanDisk
   - CreatePartitionLayout
   - FormatPartition

3. **Win32VolumeService** (2일)
   - EnumerateVolumes
   - GetVolumeInfo
   - MountVolume / DismountVolume

4. **MFTScanner** (2일)
   - FSCTL_GET_NTFS_VOLUME_DATA
   - MFT 직접 읽기
   - 파티션 내용 분석

5. **DiskTransaction** (2일)
   - Step 기반 트랜잭션
   - Rollback 구현
   - 레이아웃 백업/복원

**완료 기준**:
- ✅ 실제 디스크 열거 성공
- ✅ MFT 스캔 < 1초
- ✅ 트랜잭션 롤백 검증

### Phase 6: SMBIOS & System Info (1주)

**목표**: 시스템 정보 수집

**구현 순서**:
1. **SMBIOSStructures** (1일)
   - SMBIOS 구조체 정의

2. **SMBIOSParser** (2일)
   - GetSystemFirmwareTable
   - 파싱 로직

3. **Win32SystemInfoService** (2일)
   - GetMotherboardModel
   - GetBIOSVersion
   - IsUEFIBoot
   - GetTotalMemoryBytes

4. **통합 테스트** (2일)

**완료 기준**:
- ✅ SMBIOS 정보 정확히 파싱
- ✅ UEFI/Legacy 판별 정확도

### Phase 7: Imaging Services (2주)

**목표**: WIM 이미지 처리

**구현 순서**:
1. **WimlibAdapter** (4일)
   - wimlib 초기화
   - ApplyImage
   - CaptureImage
   - GetImageInfo

2. **WimlibOptimizer** (2일)
   - 압축 레벨 조정
   - 멀티스레드 설정
   - 메모리 제한

3. **DismAdapter** (2일)
   - DismApi 초기화
   - 드라이버 주입

4. **ProgressCallback** (1일)
   - 진행률 보고
   - 이벤트 발행

5. **통합 테스트** (3일)
   - 실제 WIM 파일 테스트
   - 성능 벤치마크

**완료 기준**:
- ✅ WIM 적용 성공
- ✅ 진행률 정확도
- ✅ Config.ini 예상 시간 준수

### Phase 8: Use Cases (1주)

**목표**: 비즈니스 로직 완성

**구현 순서**:
1. **usecases/disk** (2일)
   - EnumerateDisksUseCase
   - AnalyzeDisksUseCase
   - SelectTargetDisksUseCase

2. **usecases/install** (3일)
   - InstallWindowsUseCase
   - BackupUserDataUseCase
   - RestoreUserDataUseCase
   - InjectDriversUseCase

3. **통합 테스트** (2일)
   - End-to-End 시나리오
   - 에러 처리 검증

**완료 기준**:
- ✅ 전체 플로우 동작
- ✅ 롤백 시나리오 검증
- ✅ 모든 Use Case 테스트 통과

### Phase 9: UI Layer (1주)

**목표**: Win32 UI 구현

**구현 순서**:
1. **controls** (2일)
   - SimpleButton
   - ToggleButton

2. **Win32MainWindow** (3일)
   - 윈도우 생성
   - 메시지 루프
   - 이벤트 핸들링

3. **Win32ProgressBar** (1일)
   - 진행률 표시

4. **UI 통합** (1일)

**완료 기준**:
- ✅ UI 렌더링 정상
- ✅ 사용자 입력 처리
- ✅ 진행률 업데이트

### Phase 10: Composition Root (3일)

**목표**: 모든 것을 연결

**구현 순서**:
1. **ServiceRegistration** (1일)
   - 모든 서비스 등록
   - 의존성 와이어링

2. **Main.cpp** (1일)
   - 애플리케이션 초기화
   - 메인 플로우

3. **최종 통합 테스트** (1일)

**완료 기준**:
- ✅ WinPE 환경에서 실행
- ✅ 전체 설치 프로세스 완료
- ✅ 메모리 사용량 < 512MB

---

## 🎯 성공 기준

### 기능 요구사항
- ✅ 디스크 열거 및 분석
- ✅ 파티션 레이아웃 생성 (UEFI/Legacy)
- ✅ WIM 이미지 적용
- ✅ 드라이버 주입
- ✅ 부트로더 구성
- ✅ 사용자 데이터 백업/복원

### 비기능 요구사항
- ✅ 성능: 디스크 열거 < 3초, MFT 스캔 < 1초
- ✅ 메모리: < 512MB
- ✅ 안정성: 트랜잭션 롤백 100% 성공
- ✅ 테스트: 코드 커버리지 > 80%
- ✅ 코드 품질: 모든 경고 제거

### 아키텍처 준수
- ✅ Domain 계층 완전 격리 (Windows.h 없음)
- ✅ 의존성 규칙 위반 0건
- ✅ 모든 리소스 RAII 관리
- ✅ Expected<T> 일관된 사용

---

## 📚 참고 문서

### 필수 읽기
- `ARCHITECTURE.md` - 아키텍처 상세 설명
- `IMPLEMENTATION_GUIDE.md` - 구현 가이드라인
- `API_REFERENCE.md` - API 문서
- `PERFORMANCE_TUNING.md` - 성능 최적화 가이드

### 외부 자료
- [wimlib 문서](https://wimlib.net/documentation.html)
- [MSDN: IOCTL_DISK_*](https://docs.microsoft.com/en-us/windows/win32/api/winioctl/)
- [SMBIOS Specification](https://www.dmtf.org/standards/smbios)
- [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)

---

**계획서 작성 완료일**: 2026년 2월 15일  
**버전**: 1.0  
**작성자**: 시니어 개발자  
**상태**: 최종 승인 대기

    .FlatMap([](auto size) { return ValidateSize(size); })
    .UnwrapOr(DefaultSize);
```

### 4. 트랜잭션 규칙

```cpp
// ✅ 모든 상태 변경 작업은 트랜잭션 내에서
auto transaction = std::make_unique<DiskTransaction>(
    diskIndex, diskService, logger);

auto result = transaction->Execute([&]() -> Expected<void> {
    // Step 기반 트랜잭션 구성
    transaction->AddCleanDiskStep();
    transaction->AddCreatePartitionLayoutStep(layout);
    transaction->AddFormatPartitionStep(0, FileSystemType::NTFS);
    
    return transaction->ExecuteSteps();
});

// 실패 시 자동 롤백
if (!result.HasValue()) {
    // 원본 레이아웃 복구됨
}
```

---

## 📂 폴더 구조

```
WinSetup/
│
├── WinSetup.sln                         # Visual Studio 솔루션
├── README.md
├── ARCHITECTURE.md
│
├── WinSetup/                            # 메인 프로젝트
│   ├── WinSetup.vcxproj
│   ├── WinSetup.vcxproj.filters
│   │
│   ├── src/
│   │   │
│   │   ├── domain/                      # Layer 1: 완전 격리
│   │   │   │
│   │   │   ├── primitives/
│   │   │   │   ├── Expected.h           # ✅ Monadic 연산 포함
│   │   │   │   ├── Expected.cpp
│   │   │   │   ├── Result.h
│   │   │   │   ├── Error.h
│   │   │   │   └── Error.cpp
│   │   │   │
│   │   │   ├── memory/
│   │   │   │   ├── PoolAllocator.h      # ✅ 메모리 풀
│   │   │   │   └── PoolAllocator.cpp
│   │   │   │
│   │   │   ├── entities/
│   │   │   │   ├── DiskInfo.h
│   │   │   │   ├── DiskInfo.cpp
│   │   │   │   ├── VolumeInfo.h
│   │   │   │   ├── VolumeInfo.cpp
│   │   │   │   ├── PartitionInfo.h
│   │   │   │   ├── PartitionInfo.cpp
│   │   │   │   ├── SystemInfo.h
│   │   │   │   ├── SystemInfo.cpp
│   │   │   │   ├── SetupConfig.h
│   │   │   │   └── SetupConfig.cpp
│   │   │   │
│   │   │   ├── valueobjects/
│   │   │   │   ├── BusType.h
│   │   │   │   ├── DiskSize.h
│   │   │   │   ├── DiskSize.cpp
│   │   │   │   ├── DiskType.h
│   │   │   │   ├── PartitionType.h
│   │   │   │   ├── FileSystemType.h
│   │   │   │   ├── DriveLetter.h
│   │   │   │   └── DriveLetter.cpp
│   │   │   │
│   │   │   ├── services/
│   │   │   │   ├── DiskSortingService.h
│   │   │   │   ├── DiskSortingService.cpp
│   │   │   │   ├── PartitionAnalyzer.h
│   │   │   │   ├── PartitionAnalyzer.cpp
│   │   │   │   ├── PathNormalizer.h
│   │   │   │   └── PathNormalizer.cpp
│   │   │   │
│   │   │   ├── specifications/
│   │   │   │   ├── ISpecification.h
│   │   │   │   ├── DiskSpecifications.h
│   │   │   │   ├── DiskSpecifications.cpp
│   │   │   │   ├── VolumeSpecifications.h
│   │   │   │   └── VolumeSpecifications.cpp
│   │   │   │
│   │   │   ├── events/
│   │   │   │   ├── DomainEvent.h
│   │   │   │   ├── DomainEvent.cpp
│   │   │   │   ├── DiskAnalyzedEvent.h
│   │   │   │   ├── InstallStartedEvent.h
│   │   │   │   ├── InstallProgressEvent.h
│   │   │   │   ├── InstallCompletedEvent.h
│   │   │   │   └── ErrorOccurredEvent.h
│   │   │   │
│   │   │   └── functional/
│   │   │       ├── Monads.h
│   │   │       ├── Optional.h
│   │   │       ├── Pipeline.h
│   │   │       └── Compose.h
│   │   │
│   │   ├── abstractions/                # Layer 0: 순수 인터페이스
│   │   │   │
│   │   │   ├── repositories/
│   │   │   │   ├── IConfigRepository.h
│   │   │   │   ├── IDiskRepository.h
│   │   │   │   └── IVolumeRepository.h
│   │   │   │
│   │   │   ├── services/
│   │   │   │   ├── platform/
│   │   │   │   │   ├── ITextEncoder.h
│   │   │   │   │   ├── ISystemInfoService.h
│   │   │   │   │   └── IThreadPool.h
│   │   │   │   │
│   │   │   │   └── storage/
│   │   │   │       ├── IDiskService.h
│   │   │   │       ├── IVolumeService.h
│   │   │   │       ├── IPartitionService.h
│   │   │   │       ├── IStorageScanner.h
│   │   │   │       ├── IImagingService.h
│   │   │   │       └── IDriverService.h
│   │   │   │
│   │   │   ├── infrastructure/
│   │   │   │   ├── async/
│   │   │   │   │   ├── IExecutor.h
│   │   │   │   │   ├── IScheduler.h
│   │   │   │   │   ├── IThreadPool.h
│   │   │   │   │   └── IAsyncContext.h
│   │   │   │   │
│   │   │   │   ├── messaging/
│   │   │   │   │   ├── IEvent.h
│   │   │   │   │   ├── IEventBus.h
│   │   │   │   │   ├── IDispatcher.h
│   │   │   │   │   └── IMessageQueue.h
│   │   │   │   │
│   │   │   │   ├── logging/
│   │   │   │   │   ├── ILogger.h
│   │   │   │   │   └── LogLevel.h
│   │   │   │   │
│   │   │   │   └── transaction/
│   │   │   │       ├── ITransaction.h
│   │   │   │       └── ITransactionManager.h
│   │   │   │
│   │   │   └── ui/
│   │   │       ├── IWindow.h
│   │   │       ├── IWidget.h
│   │   │       └── IProgressBar.h
│   │   │
│   │   ├── application/                 # Layer 2: Use Cases
│   │   │   │
│   │   │   ├── core/
│   │   │   │   ├── DIContainer.h        # ✅ DI Container
│   │   │   │   ├── DIContainer.cpp
│   │   │   │   ├── ServiceLocator.h
│   │   │   │   └── ServiceLocator.cpp
│   │   │   │
│   │   │   ├── async/
│   │   │   │   ├── Task.h               # ✅ 코루틴 완전 구현
│   │   │   │   ├── Task.cpp
│   │   │   │   ├── Promise.h
│   │   │   │   ├── Awaitable.h
│   │   │   │   ├── CancellationToken.h
│   │   │   │   └── CancellationToken.cpp
│   │   │   │
│   │   │   ├── usecases/
│   │   │   │   ├── system/
│   │   │   │   │   ├── AnalyzeSystemUseCase.h
│   │   │   │   │   ├── AnalyzeSystemUseCase.cpp
│   │   │   │   │   ├── LoadConfigurationUseCase.h
│   │   │   │   │   └── LoadConfigurationUseCase.cpp
│   │   │   │   │
│   │   │   │   ├── disk/
│   │   │   │   │   ├── EnumerateDisksUseCase.h
│   │   │   │   │   ├── EnumerateDisksUseCase.cpp
│   │   │   │   │   ├── AnalyzeDisksUseCase.h
│   │   │   │   │   ├── AnalyzeDisksUseCase.cpp
│   │   │   │   │   ├── SelectTargetDisksUseCase.h
│   │   │   │   │   └── SelectTargetDisksUseCase.cpp
│   │   │   │   │
│   │   │   │   └── install/
│   │   │   │       ├── InstallWindowsUseCase.h
│   │   │   │       ├── InstallWindowsUseCase.cpp
│   │   │   │       ├── BackupUserDataUseCase.h
│   │   │   │       ├── BackupUserDataUseCase.cpp
│   │   │   │       ├── RestoreUserDataUseCase.h
│   │   │   │       ├── RestoreUserDataUseCase.cpp
│   │   │   │       ├── ApplyImageUseCase.h
│   │   │   │       ├── ApplyImageUseCase.cpp
│   │   │   │       ├── InjectDriversUseCase.h
│   │   │   │       └── InjectDriversUseCase.cpp
│   │   │   │
│   │   │   ├── services/
│   │   │   │   ├── TaskScheduler.h
│   │   │   │   ├── TaskScheduler.cpp
│   │   │   │   ├── EventBus.h
│   │   │   │   ├── EventBus.cpp
│   │   │   │   ├── Dispatcher.h
│   │   │   │   └── Dispatcher.cpp
│   │   │   │
│   │   │   ├── eventhandlers/
│   │   │   │   ├── DiskAnalyzedEventHandler.h
│   │   │   │   ├── DiskAnalyzedEventHandler.cpp
│   │   │   │   ├── InstallProgressEventHandler.h
│   │   │   │   └── InstallProgressEventHandler.cpp
│   │   │   │
│   │   │   └── dto/
│   │   │       ├── SystemAnalysisResult.h
│   │   │       ├── DiskAnalysisResult.h
│   │   │       └── InstallationProgress.h
│   │   │
│   │   ├── adapters/                    # Layer 3: 저수준 구현
│   │   │   │
│   │   │   ├── platform/
│   │   │   │   └── win32/
│   │   │   │       ├── core/
│   │   │   │       │   ├── Win32TypeMapper.h
│   │   │   │       │   ├── Win32TypeMapper.cpp
│   │   │   │       │   ├── Win32ErrorHandler.h
│   │   │   │       │   ├── Win32ErrorHandler.cpp
│   │   │   │       │   ├── Win32StringHelper.h
│   │   │   │       │   ├── Win32StringHelper.cpp
│   │   │   │       │   ├── Win32HandleFactory.h
│   │   │   │       │   ├── Win32HandleFactory.cpp
│   │   │   │       │   └── Win32Constants.h
│   │   │   │       │
│   │   │   │       ├── memory/
│   │   │   │       │   ├── UniqueHandle.h      # ✅ RAII 핸들 래퍼
│   │   │   │       │   ├── UniqueHandle.cpp
│   │   │   │       │   ├── UniqueLibrary.h
│   │   │   │       │   └── UniqueFindHandle.h
│   │   │   │       │
│   │   │   │       ├── system/
│   │   │   │       │   ├── Win32SystemInfoService.h
│   │   │   │       │   ├── Win32SystemInfoService.cpp
│   │   │   │       │   ├── SMBIOSParser.h
│   │   │   │       │   ├── SMBIOSParser.cpp
│   │   │   │       │   ├── SMBIOSStructures.h
│   │   │   │       │   └── FirmwareTableReader.h
│   │   │   │       │
│   │   │   │       ├── storage/
│   │   │   │       │   ├── Win32DiskService.h
│   │   │   │       │   ├── Win32DiskService.cpp
│   │   │   │       │   ├── Win32VolumeService.h
│   │   │   │       │   ├── Win32VolumeService.cpp
│   │   │   │       │   ├── IOCTLWrapper.h
│   │   │   │       │   ├── IOCTLWrapper.cpp
│   │   │   │       │   ├── AsyncIOCTL.h      # ✅ OVERLAPPED 비동기
│   │   │   │       │   ├── AsyncIOCTL.cpp
│   │   │   │       │   ├── MFTScanner.h      # ✅ MFT 직접 읽기
│   │   │   │       │   ├── MFTScanner.cpp
│   │   │   │       │   ├── DiskTransaction.h # ✅ Step-based 트랜잭션
│   │   │   │       │   ├── DiskTransaction.cpp
│   │   │   │       │   ├── DiskLayoutBuilder.h
│   │   │   │       │   └── DiskLayoutBuilder.cpp
│   │   │   │       │
│   │   │   │       ├── threading/
│   │   │   │       │   ├── Win32ThreadPool.h
│   │   │   │       │   ├── Win32ThreadPool.cpp
│   │   │   │       │   ├── Win32Thread.h
│   │   │   │       │   └── Win32Thread.cpp
│   │   │   │       │
│   │   │   │       └── logging/
│   │   │   │           ├── Win32Logger.h     # ✅ 버퍼링 로거
│   │   │   │           └── Win32Logger.cpp
│   │   │   │
│   │   │   ├── persistence/
│   │   │   │   ├── config/
│   │   │   │   │   ├── IniConfigRepository.h
│   │   │   │   │   ├── IniConfigRepository.cpp
│   │   │   │   │   ├── IniParser.h
│   │   │   │   │   └── IniParser.cpp
│   │   │   │   │
│   │   │   │   └── filesystem/
│   │   │   │       ├── Win32FileSystem.h
│   │   │   │       └── Win32FileSystem.cpp
│   │   │   │
│   │   │   ├── imaging/
│   │   │   │   ├── WimlibAdapter.h
│   │   │   │   ├── WimlibAdapter.cpp
│   │   │   │   ├── WimlibOptimizer.h     # ✅ wimlib 최적화
│   │   │   │   ├── WimlibOptimizer.cpp
│   │   │   │   ├── DismAdapter.h
│   │   │   │   └── DismAdapter.cpp
│   │   │   │
│   │   │   └── ui/
│   │   │       └── win32/
│   │   │           ├── controls/
│   │   │           │   ├── SimpleButton.h
│   │   │           │   ├── SimpleButton.cpp
│   │   │           │   ├── ToggleButton.h
│   │   │           │   └── ToggleButton.cpp
│   │   │           ├── Win32MainWindow.h
│   │   │           ├── Win32MainWindow.cpp
│   │   │           ├── Win32ProgressBar.h
│   │   │           └── Win32ProgressBar.cpp
│   │   │
│   │   ├── main/                        # Layer 4: Composition Root
│   │   │   ├── Main.cpp
│   │   │   ├── ServiceRegistration.h
│   │   │   └── ServiceRegistration.cpp
│   │   │
│   │   ├── resources/
│   │   │   ├── resource.h
│   │   │   ├── app.rc
│   │   │   └── app.manifest
│   │   │
│   │   └── lib/
│   │       ├── wimlib.h
│   │       ├── wimlib.lib
│   │       └── wimlib.dll
│
├── WinSetup.Tests/                      # 테스트 프로젝트
│   ├── WinSetup.Tests.vcxproj
│   │
│   ├── domain/
│   │   ├── primitives/
│   │   │   ├── ExpectedTests.cpp
│   │   │   └── ErrorTests.cpp
│   │   │
│   │   ├── entities/
│   │   │   ├── DiskInfoTests.cpp
│   │   │   └── SetupConfigTests.cpp
│   │   │
│   │   ├── services/
│   │   │   ├── DiskSortingServiceTests.cpp
│   │   │   └── PartitionAnalyzerTests.cpp
│   │   │
│   │   └── specifications/
│   │       └── DiskSpecificationsTests.cpp
│   │
│   ├── application/
│   │   ├── core/
│   │   │   └── DIContainerTests.cpp
│   │   │
│   │   ├── usecases/
│   │   │   ├── AnalyzeSystemUseCaseTests.cpp
│   │   │   └── InstallWindowsUseCaseTests.cpp
│   │   │
│   │   └── services/
│   │       └── EventBusTests.cpp
│   │
│   ├── adapters/
│   │   ├── Win32DiskServiceTests.cpp
│   │   ├── MFTScannerTests.cpp
│   │   ├── DiskTransactionTests.cpp
│   │   └── SMBIOSParserTests.cpp
│   │
│   ├── integration/
│   │   ├── EndToEndTests.cpp
│   │   └── PerformanceTests.cpp
│   │
│   ├── mocks/
│   │   ├── MockDiskService.h
│   │   ├── MockEventBus.h
│   │   └── MockLogger.h
│   │
│   └── TestMain.cpp
│
└── docs/
    ├── IMPLEMENTATION_GUIDE.md
    ├── API_REFERENCE.md
    └── PERFORMANCE_TUNING.md
```

---

## 🔧 Domain 계층 설계 (완전 격리)

### Expected<T> - Monadic 에러 처리

#### domain/primitives/Expected.h
```cpp
#pragma once

#include <utility>
#include <type_traits>
#include <cassert>
#include "Error.h"

namespace winsetup::domain {

    template<typename T>
    class Expected {
    public:
        Expected(T value)
            : mHasValue(true)
        {
            new (&mValue) T(std::move(value));
        }

        Expected(Error error)
            : mHasValue(false)
        {
            new (&mError) Error(std::move(error));
        }

        ~Expected() {
            if (mHasValue) {
                mValue.~T();
            } else {
                mError.~Error();
            }
        }

        Expected(const Expected& other)
            : mHasValue(other.mHasValue)
        {
            if (mHasValue) {
                new (&mValue) T(other.mValue);
            } else {
                new (&mError) Error(other.mError);
            }
        }

        Expected(Expected&& other) noexcept
            : mHasValue(other.mHasValue)
        {
            if (mHasValue) {
                new (&mValue) T(std::move(other.mValue));
            } else {
                new (&mError) Error(std::move(other.mError));
            }
        }

        Expected& operator=(const Expected& other) {
            if (this != &other) {
                this->~Expected();
                new (this) Expected(other);
            }
            return *this;
        }

        Expected& operator=(Expected&& other) noexcept {
            if (this != &other) {
                this->~Expected();
                new (this) Expected(std::move(other));
            }
            return *this;
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return mHasValue;
        }

        [[nodiscard]] T& Value() & {
            assert(mHasValue && "Expected must contain value");
            return mValue;
        }

        [[nodiscard]] const T& Value() const & {
            assert(mHasValue && "Expected must contain value");
            return mValue;
        }

        [[nodiscard]] T&& Value() && {
            assert(mHasValue && "Expected must contain value");
            return std::move(mValue);
        }

        [[nodiscard]] T& ValueOr(T& defaultValue) & noexcept {
            return mHasValue ? mValue : defaultValue;
        }

        [[nodiscard]] const Error& GetError() const {
            assert(!mHasValue && "Expected must contain error");
            return mError;
        }

        template<typename F>
        [[nodiscard]] auto Map(F&& func) -> Expected<decltype(func(std::declval<T>()))> {
            using U = decltype(func(std::declval<T>()));

            if (mHasValue) {
                return Expected<U>(func(mValue));
            } else {
                return Expected<U>(mError);
            }
        }

        template<typename F>
        [[nodiscard]] auto FlatMap(F&& func) -> decltype(func(std::declval<T>())) {
            using Result = decltype(func(std::declval<T>()));

            if (mHasValue) {
                return func(mValue);
            } else {
                return Result(mError);
            }
        }

        [[nodiscard]] T UnwrapOr(T defaultValue) && {
            if (mHasValue) {
                return std::move(mValue);
            } else {
                return defaultValue;
            }
        }

        template<typename F>
        [[nodiscard]] T UnwrapOrElse(F&& func) && {
            if (mHasValue) {
                return std::move(mValue);
            } else {
                return func(mError);
            }
        }

        template<typename F>
        Expected& OnError(F&& func) & {
            if (!mHasValue) {
                func(mError);
            }
            return *this;
        }

    private:
        union {
            T mValue;
            Error mError;
        };
        bool mHasValue;
    };

    template<>
    class Expected<void> {
    public:
        Expected() : mHasValue(true) {}

        Expected(Error error)
            : mHasValue(false)
            , mError(std::move(error))
        {}

        [[nodiscard]] bool HasValue() const noexcept {
            return mHasValue;
        }

        [[nodiscard]] const Error& GetError() const {
            assert(!mHasValue && "Expected must contain error");
            return mError;
        }

        template<typename F>
        Expected& OnError(F&& func) & {
            if (!mHasValue) {
                func(mError);
            }
            return *this;
        }

    private:
        bool mHasValue;
        Error mError;
    };

}
```

### Error - 구조화된 에러

#### domain/primitives/Error.h
```cpp
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

namespace winsetup::domain {

    enum class ErrorCategory {
        Unknown,
        System,
        Disk,
        Volume,
        Imaging,
        Configuration,
        Network,
        Permission
    };

    struct ErrorContext {
        std::wstring function;
        std::wstring file;
        int line;
        std::chrono::system_clock::time_point timestamp;
    };

    class Error {
    public:
        Error(
            std::wstring message,
            uint32_t code = 0,
            ErrorCategory category = ErrorCategory::Unknown
        )
            : mMessage(std::move(message))
            , mCode(code)
            , mCategory(category)
            , mTimestamp(std::chrono::system_clock::now())
        {}

        [[nodiscard]] const std::wstring& GetMessage() const noexcept {
            return mMessage;
        }

        [[nodiscard]] uint32_t GetCode() const noexcept {
            return mCode;
        }

        [[nodiscard]] ErrorCategory GetCategory() const noexcept {
            return mCategory;
        }

        [[nodiscard]] auto GetTimestamp() const noexcept {
            return mTimestamp;
        }

        void AddContext(const ErrorContext& context) {
            mContexts.push_back(context);
        }

        [[nodiscard]] const std::vector<ErrorContext>& GetContexts() const noexcept {
            return mContexts;
        }

        [[nodiscard]] std::wstring ToString() const;

    private:
        std::wstring mMessage;
        uint32_t mCode;
        ErrorCategory mCategory;
        std::chrono::system_clock::time_point mTimestamp;
        std::vector<ErrorContext> mContexts;
    };

}
```

### PoolAllocator - 메모리 풀

#### domain/memory/PoolAllocator.h
```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

namespace winsetup::domain {

    template<typename T, size_t BlockSize = 4096>
    class PoolAllocator {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        PoolAllocator() noexcept = default;

        template<typename U>
        PoolAllocator(const PoolAllocator<U, BlockSize>&) noexcept {}

        [[nodiscard]] pointer allocate(size_type n) {
            if (n == 0) {
                return nullptr;
            }

            if (n > BlockSize) {
                return static_cast<pointer>(::operator new(n * sizeof(T)));
            }

            std::lock_guard lock(mMutex);

            if (mFreeList.empty()) {
                AllocateBlock();
            }

            pointer result = mFreeList.back();
            mFreeList.pop_back();
            return result;
        }

        void deallocate(pointer p, size_type n) noexcept {
            if (!p) {
                return;
            }

            if (n > BlockSize) {
                ::operator delete(p);
                return;
            }

            std::lock_guard lock(mMutex);
            mFreeList.push_back(p);
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            new (p) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) {
            p->~U();
        }

    private:
        void AllocateBlock() {
            auto block = std::make_unique<std::byte[]>(BlockSize * sizeof(T));
            
            for (size_t i = 0; i < BlockSize; ++i) {
                mFreeList.push_back(
                    reinterpret_cast<pointer>(block.get() + i * sizeof(T))
                );
            }

            mBlocks.push_back(std::move(block));
        }

        std::vector<pointer> mFreeList;
        std::vector<std::unique_ptr<std::byte[]>> mBlocks;
        std::mutex mMutex;
    };

}
```

### DiskSortingService - 도메인 서비스

#### domain/services/DiskSortingService.h
```cpp
#pragma once

#include "../entities/DiskInfo.h"
#include <vector>

namespace winsetup::domain {

    struct FilterAndSortResult {
        std::vector<DiskInfo> installableDisks;
        std::vector<DiskInfo> filteredDisks;
        size_t totalDisks;

        FilterAndSortResult()
            : totalDisks(0)
        {}
    };

    class DiskSortingService {
    public:
        [[nodiscard]] static FilterAndSortResult FilterAndSort(
            const std::vector<DiskInfo>& disks
        );

    private:
        [[nodiscard]] static bool IsInstallable(const DiskInfo& disk);
        static void SortByPreference(std::vector<DiskInfo>& disks);
    };

}
```

### PartitionAnalyzer - 파티션 분석

#### domain/services/PartitionAnalyzer.h
```cpp
#pragma once

#include "../entities/PartitionInfo.h"
#include "../primitives/Expected.h"
#include <vector>
#include <optional>

namespace winsetup::domain {

    struct PartitionAnalysisResult {
        bool hasEFIPartition;
        bool hasMSRPartition;
        bool hasWindowsPartition;
        bool hasRecoveryPartition;
        std::optional<PartitionInfo> efiPartition;
        std::optional<PartitionInfo> msrPartition;
        std::optional<PartitionInfo> windowsPartition;
        std::optional<PartitionInfo> recoveryPartition;
        uint32_t totalPartitions;

        PartitionAnalysisResult()
            : hasEFIPartition(false)
            , hasMSRPartition(false)
            , hasWindowsPartition(false)
            , hasRecoveryPartition(false)
            , totalPartitions(0)
        {}
    };

    class PartitionAnalyzer {
    public:
        [[nodiscard]] static Expected<PartitionAnalysisResult> Analyze(
            const std::vector<PartitionInfo>& partitions
        );

        [[nodiscard]] static bool IsEFIPartition(const PartitionInfo& partition);
        [[nodiscard]] static bool IsMSRPartition(const PartitionInfo& partition);
        [[nodiscard]] static bool IsWindowsPartition(const PartitionInfo& partition);
        [[nodiscard]] static bool IsRecoveryPartition(const PartitionInfo& partition);
    };

}
```

---

## 📡 Abstractions 계층 설계 (순수 인터페이스)

### IConfigRepository

#### abstractions/repositories/IConfigRepository.h
```cpp
#pragma once

#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/SetupConfig.h"
#include <string>

namespace winsetup::abstractions {

    class IConfigRepository {
    public:
        virtual ~IConfigRepository() = default;

        [[nodiscard]] virtual domain::Expected<domain::SetupConfig>
            Load(const std::wstring& path) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            Save(const std::wstring& path, const domain::SetupConfig& config) = 0;

        [[nodiscard]] virtual domain::Expected<bool>
            Exists(const std::wstring& path) const = 0;
    };

}
```

### IDiskService

#### abstractions/services/storage/IDiskService.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/DiskInfo.h"
#include "../../../domain/entities/PartitionInfo.h"
#include "../../../domain/valueobjects/FileSystemType.h"
#include <vector>
#include <cstdint>

namespace winsetup::abstractions {

    struct PartitionLayout {
        enum class Style {
            MBR,
            GPT
        };

        Style style;
        std::vector<domain::PartitionInfo> partitions;

        [[nodiscard]] bool IsValid() const noexcept {
            return !partitions.empty();
        }
    };

    class IDiskService {
    public:
        virtual ~IDiskService() = default;

        [[nodiscard]] virtual domain::Expected<std::vector<domain::DiskInfo>>
            EnumerateDisks() = 0;

        [[nodiscard]] virtual domain::Expected<domain::DiskInfo>
            GetDiskInfo(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            CleanDisk(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            CreatePartitionLayout(
                uint32_t diskIndex,
                const PartitionLayout& layout
            ) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            FormatPartition(
                uint32_t diskIndex,
                uint32_t partitionIndex,
                domain::FileSystemType fileSystem,
                bool quickFormat = true
            ) = 0;

        [[nodiscard]] virtual domain::Expected<PartitionLayout>
            GetCurrentLayout(uint32_t diskIndex) = 0;

        [[nodiscard]] virtual domain::Expected<void>
            RestoreLayout(
                uint32_t diskIndex,
                const PartitionLayout& layout
            ) = 0;
    };

}
```

### ILogger

#### abstractions/infrastructure/logging/ILogger.h
```cpp
#pragma once

#include <string>
#include <source_location>
#include "LogLevel.h"

namespace winsetup::abstractions {

    class ILogger {
    public:
        virtual ~ILogger() = default;

        virtual void Log(
            LogLevel level,
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) = 0;

        void Trace(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Trace, message, location);
        }

        void Debug(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Debug, message, location);
        }

        void Info(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Info, message, location);
        }

        void Warning(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Warning, message, location);
        }

        void Error(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Error, message, location);
        }

        void Fatal(
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) {
            Log(LogLevel::Fatal, message, location);
        }

        virtual void Flush() = 0;
    };

}
```

#### abstractions/infrastructure/logging/LogLevel.h
```cpp
#pragma once

namespace winsetup::abstractions {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

}
```

### ITransaction

#### abstractions/infrastructure/transaction/ITransaction.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include <functional>
#include <string>

namespace winsetup::abstractions {

    enum class TransactionState {
        NotStarted,
        Active,
        Committed,
        RolledBack,
        Failed
    };

    class ITransaction {
    public:
        virtual ~ITransaction() = default;

        [[nodiscard]] virtual domain::Expected<void> Begin() = 0;
        [[nodiscard]] virtual domain::Expected<void> Commit() = 0;
        [[nodiscard]] virtual domain::Expected<void> Rollback() = 0;

        [[nodiscard]] virtual bool IsActive() const noexcept = 0;
        [[nodiscard]] virtual TransactionState GetState() const noexcept = 0;

        [[nodiscard]] virtual domain::Expected<void> Execute(
            std::function<domain::Expected<void>()> operation
        ) = 0;

        virtual void SetAutoRollback(bool enabled) noexcept = 0;
        virtual void SetTimeout(uint32_t timeoutMs) noexcept = 0;

        [[nodiscard]] virtual std::wstring GetTransactionLog() const = 0;
    };

}
```

---


## 🎮 Application 계층 설계

### DIContainer - 의존성 주입

#### application/core/DIContainer.h
```cpp
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <any>
#include <mutex>

namespace winsetup::application {

    enum class ServiceLifetime {
        Singleton,
        Transient,
        Scoped
    };

    class DIContainer {
    public:
        template<typename TInterface, typename TImplementation>
        void Register(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>();
            };

            mRegistrations[typeid(TInterface)] = {factory, lifetime};
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>(
                    Resolve<TDeps>()...
                );
            };

            mRegistrations[typeid(TInterface)] = {factory, lifetime};
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::lock_guard lock(mMutex);

            mSingletons[typeid(TInterface)] = instance;

            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
            };

            mRegistrations[typeid(TInterface)] = {factory, ServiceLifetime::Singleton};
        }

        template<typename TInterface>
        std::shared_ptr<TInterface> Resolve() {
            std::lock_guard lock(mMutex);

            auto typeIndex = typeid(TInterface);

            if (auto it = mRegistrations.find(typeIndex); it != mRegistrations.end()) {
                const auto& [factory, lifetime] = it->second;

                if (lifetime == ServiceLifetime::Singleton) {
                    if (auto singletonIt = mSingletons.find(typeIndex); 
                        singletonIt != mSingletons.end()) {
                        return std::static_pointer_cast<TInterface>(singletonIt->second);
                    }

                    auto instance = factory();
                    mSingletons[typeIndex] = instance;
                    return std::static_pointer_cast<TInterface>(instance);
                }

                return std::static_pointer_cast<TInterface>(factory());
            }

            return nullptr;
        }

        void Clear() {
            std::lock_guard lock(mMutex);
            mSingletons.clear();
            mRegistrations.clear();
        }

    private:
        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime;
        };

        std::unordered_map<std::type_index, Registration> mRegistrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;
        std::mutex mMutex;
    };

}
```

### Task<T> - 코루틴 구현

#### application/async/Task.h
```cpp
#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include "../../domain/primitives/Expected.h"

namespace winsetup::application {

    template<typename T>
    class Task {
    public:
        struct promise_type {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_value(T value) {
                mResult = domain::Expected<T>(std::move(value));
            }

            void unhandled_exception() {
                mException = std::current_exception();
            }

            domain::Expected<T> mResult{domain::Error{L"Not completed", 0}};
            std::exception_ptr mException;
        };

        Task(std::coroutine_handle<promise_type> handle)
            : mHandle(handle)
        {}

        ~Task() {
            if (mHandle) {
                mHandle.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept
            : mHandle(std::exchange(other.mHandle, {}))
        {}

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (mHandle) {
                    mHandle.destroy();
                }
                mHandle = std::exchange(other.mHandle, {});
            }
            return *this;
        }

        void Resume() {
            if (mHandle && !mHandle.done()) {
                mHandle.resume();
            }
        }

        bool IsDone() const {
            return !mHandle || mHandle.done();
        }

        domain::Expected<T> GetResult() {
            if (!IsDone()) {
                return domain::Error{L"Task not completed", 0};
            }

            if (mHandle.promise().mException) {
                std::rethrow_exception(mHandle.promise().mException);
            }

            return std::move(mHandle.promise().mResult);
        }

    private:
        std::coroutine_handle<promise_type> mHandle;
    };

    template<>
    class Task<void> {
    public:
        struct promise_type {
            Task get_return_object() {
                return Task{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_void() {}

            void unhandled_exception() {
                mException = std::current_exception();
            }

            std::exception_ptr mException;
        };

        Task(std::coroutine_handle<promise_type> handle)
            : mHandle(handle)
        {}

        ~Task() {
            if (mHandle) {
                mHandle.destroy();
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept
            : mHandle(std::exchange(other.mHandle, {}))
        {}

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (mHandle) {
                    mHandle.destroy();
                }
                mHandle = std::exchange(other.mHandle, {});
            }
            return *this;
        }

        void Resume() {
            if (mHandle && !mHandle.done()) {
                mHandle.resume();
            }
        }

        bool IsDone() const {
            return !mHandle || mHandle.done();
        }

    private:
        std::coroutine_handle<promise_type> mHandle;
    };

}
```

### InstallWindowsUseCase - 핵심 유즈케이스

#### application/usecases/install/InstallWindowsUseCase.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/SetupConfig.h"
#include "../../../domain/entities/DiskInfo.h"
#include "../../../abstractions/services/storage/IDiskService.h"
#include "../../../abstractions/services/storage/IImagingService.h"
#include "../../../abstractions/infrastructure/logging/ILogger.h"
#include "../../../abstractions/infrastructure/messaging/IEventBus.h"
#include "../../async/Task.h"
#include <memory>

namespace winsetup::application {

    class InstallWindowsUseCase {
    public:
        InstallWindowsUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IImagingService> imagingService,
            std::shared_ptr<abstractions::ILogger> logger,
            std::shared_ptr<abstractions::IEventBus> eventBus
        );

        [[nodiscard]] Task<domain::Expected<void>> Execute(
            const domain::SetupConfig& config,
            const domain::DiskInfo& targetDisk
        );

    private:
        [[nodiscard]] domain::Expected<void> PrepareTargetDisk(
            uint32_t diskIndex,
            bool isUEFI
        );

        [[nodiscard]] domain::Expected<void> ApplyWindowsImage(
            const std::wstring& wimPath,
            uint32_t imageIndex,
            wchar_t targetDrive
        );

        [[nodiscard]] domain::Expected<void> ConfigureBootLoader(
            wchar_t systemDrive,
            wchar_t efiDrive,
            bool isUEFI
        );

        void PublishProgress(
            const std::wstring& stage,
            uint32_t percentage
        );

        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::IImagingService> mImagingService;
        std::shared_ptr<abstractions::ILogger> mLogger;
        std::shared_ptr<abstractions::IEventBus> mEventBus;
    };

}
```

---

## 🔌 Adapters 계층 설계 (저수준 구현)

### UniqueHandle - Win32 RAII 래퍼

#### adapters/platform/win32/memory/UniqueHandle.h
```cpp
#pragma once

#include <Windows.h>
#include <utility>

namespace winsetup::adapters::win32 {

    class UniqueHandle {
    public:
        explicit UniqueHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept
            : mHandle(handle)
        {}

        ~UniqueHandle() noexcept {
            Close();
        }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept
            : mHandle(std::exchange(other.mHandle, INVALID_HANDLE_VALUE))
        {}

        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Close();
                mHandle = std::exchange(other.mHandle, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return mHandle;
        }

        [[nodiscard]] HANDLE Release() noexcept {
            return std::exchange(mHandle, INVALID_HANDLE_VALUE);
        }

        void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept {
            Close();
            mHandle = handle;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHandle != INVALID_HANDLE_VALUE && mHandle != nullptr;
        }

        [[nodiscard]] HANDLE* AddressOf() noexcept {
            return &mHandle;
        }

    private:
        void Close() noexcept {
            if (mHandle != INVALID_HANDLE_VALUE && mHandle != nullptr) {
                CloseHandle(mHandle);
                mHandle = INVALID_HANDLE_VALUE;
            }
        }

        HANDLE mHandle;
    };

    class UniqueLibrary {
    public:
        explicit UniqueLibrary(HMODULE hModule = nullptr) noexcept
            : mHModule(hModule)
        {}

        ~UniqueLibrary() noexcept {
            if (mHModule) {
                FreeLibrary(mHModule);
            }
        }

        UniqueLibrary(const UniqueLibrary&) = delete;
        UniqueLibrary& operator=(const UniqueLibrary&) = delete;

        UniqueLibrary(UniqueLibrary&& other) noexcept
            : mHModule(std::exchange(other.mHModule, nullptr))
        {}

        UniqueLibrary& operator=(UniqueLibrary&& other) noexcept {
            if (this != &other) {
                if (mHModule) {
                    FreeLibrary(mHModule);
                }
                mHModule = std::exchange(other.mHModule, nullptr);
            }
            return *this;
        }

        [[nodiscard]] HMODULE Get() const noexcept {
            return mHModule;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHModule != nullptr;
        }

    private:
        HMODULE mHModule;
    };

    class UniqueFindHandle {
    public:
        explicit UniqueFindHandle(HANDLE hFind = INVALID_HANDLE_VALUE) noexcept
            : mHFind(hFind)
        {}

        ~UniqueFindHandle() noexcept {
            if (mHFind != INVALID_HANDLE_VALUE) {
                FindClose(mHFind);
            }
        }

        UniqueFindHandle(const UniqueFindHandle&) = delete;
        UniqueFindHandle& operator=(const UniqueFindHandle&) = delete;

        UniqueFindHandle(UniqueFindHandle&& other) noexcept
            : mHFind(std::exchange(other.mHFind, INVALID_HANDLE_VALUE))
        {}

        UniqueFindHandle& operator=(UniqueFindHandle&& other) noexcept {
            if (this != &other) {
                if (mHFind != INVALID_HANDLE_VALUE) {
                    FindClose(mHFind);
                }
                mHFind = std::exchange(other.mHFind, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return mHFind;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return mHFind != INVALID_HANDLE_VALUE;
        }

    private:
        HANDLE mHFind;
    };

}
```

### Win32StringHelper - 문자열 유틸리티

#### adapters/platform/win32/core/Win32StringHelper.h
```cpp
#pragma once

#include <string>
#include <Windows.h>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    class Win32StringHelper {
    public:
        [[nodiscard]] static domain::Expected<std::wstring> UTF8ToWide(
            const std::string& utf8
        );

        [[nodiscard]] static domain::Expected<std::string> WideToUTF8(
            const std::wstring& wide
        );

        [[nodiscard]] static std::wstring FormatErrorMessage(DWORD errorCode);

        [[nodiscard]] static std::wstring NormalizePath(
            const std::wstring& path
        );

        [[nodiscard]] static std::wstring GetVolumePath(
            const std::wstring& path
        );

        [[nodiscard]] static bool IsAbsolutePath(const std::wstring& path);

        static std::wstring ToUpper(const std::wstring& str);
        static std::wstring ToLower(const std::wstring& str);
    };

}
```

### Win32HandleFactory - 핸들 팩토리

#### adapters/platform/win32/core/Win32HandleFactory.h
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>
#include "../../../../domain/primitives/Expected.h"
#include "../memory/UniqueHandle.h"

namespace winsetup::adapters::win32 {

    class Win32HandleFactory {
    public:
        [[nodiscard]] static domain::Expected<UniqueHandle> OpenDisk(
            uint32_t diskIndex,
            DWORD accessFlags = GENERIC_READ | GENERIC_WRITE
        );

        [[nodiscard]] static domain::Expected<UniqueHandle> OpenVolume(
            const std::wstring& volumePath,
            DWORD accessFlags = GENERIC_READ
        );

        [[nodiscard]] static domain::Expected<UniqueHandle> OpenFile(
            const std::wstring& filePath,
            DWORD desiredAccess,
            DWORD shareMode,
            DWORD creationDisposition
        );

    private:
        static std::wstring BuildDiskPath(uint32_t diskIndex);
        static std::wstring BuildVolumePath(const std::wstring& volumePath);
    };

}
```

### DiskTransaction - Step 기반 트랜잭션

#### adapters/platform/win32/storage/DiskTransaction.h
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>
#include "../../../../domain/primitives/Expected.h"
#include "../../../../abstractions/services/storage/IDiskService.h"
#include "../../../../abstractions/infrastructure/logging/ILogger.h"

namespace winsetup::adapters::win32 {

    class DiskTransaction {
    public:
        DiskTransaction(
            uint32_t diskIndex,
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<void> Execute(
            std::function<domain::Expected<void>()> operation
        );

        void AddStep(
            const std::wstring& description,
            std::function<domain::Expected<void>()> execute,
            std::function<domain::Expected<void>()> rollback
        );

        void AddCleanDiskStep();
        
        void AddCreatePartitionLayoutStep(
            const abstractions::PartitionLayout& layout
        );

        void AddFormatPartitionStep(
            uint32_t partitionIndex,
            domain::FileSystemType fileSystem,
            bool quickFormat = true
        );

        [[nodiscard]] domain::Expected<void> ExecuteSteps();

        void SetTimeout(uint32_t timeoutMs) { mTimeoutMs = timeoutMs; }
        void SetAutoRollback(bool enabled) { mAutoRollback = enabled; }

    private:
        struct TransactionStep {
            std::wstring description;
            std::function<domain::Expected<void>()> execute;
            std::function<domain::Expected<void>()> rollback;
            bool executed;
            std::chrono::system_clock::time_point timestamp;

            TransactionStep(
                std::wstring desc,
                std::function<domain::Expected<void>()> exec,
                std::function<domain::Expected<void>()> roll
            )
                : description(std::move(desc))
                , execute(std::move(exec))
                , rollback(std::move(roll))
                , executed(false)
            {}
        };

        [[nodiscard]] domain::Expected<void> BackupCurrentLayout();
        [[nodiscard]] domain::Expected<void> RestoreBackupLayout();
        [[nodiscard]] domain::Expected<void> RollbackSteps();

        void LogStep(const std::wstring& message);
        bool CheckTimeout() const;

        uint32_t mDiskIndex;
        std::shared_ptr<abstractions::IDiskService> mDiskService;
        std::shared_ptr<abstractions::ILogger> mLogger;

        std::vector<TransactionStep> mSteps;
        std::optional<abstractions::PartitionLayout> mBackupLayout;
        bool mLayoutBackedUp;

        uint32_t mTimeoutMs;
        bool mAutoRollback;
        std::chrono::system_clock::time_point mStartTime;
        std::vector<std::wstring> mTransactionLog;
    };

}
```

### Win32Logger - 버퍼링 로거

#### adapters/platform/win32/logging/Win32Logger.h
```cpp
#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <source_location>
#include "../../../../abstractions/infrastructure/logging/ILogger.h"
#include "../../../../abstractions/infrastructure/logging/LogLevel.h"

namespace winsetup::adapters::win32 {

    class Win32Logger : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath);
        ~Win32Logger() override;

        void Log(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location = std::source_location::current()
        ) override;

        void Flush() override;

        void SetMinimumLevel(abstractions::LogLevel level) {
            mMinLevel = level;
        }

        void SetBufferSize(size_t size) {
            mBufferSize = size;
        }

    private:
        void FlushBuffer();
        void FlushIfNeeded(abstractions::LogLevel level);
        
        std::wstring FormatLogEntry(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location
        );

        std::wstring LevelToString(abstractions::LogLevel level);

        static constexpr size_t DEFAULT_BUFFER_SIZE = 100;

        std::wstring mLogFilePath;
        std::wofstream mFileStream;
        std::vector<std::wstring> mBuffer;
        size_t mBufferSize;
        abstractions::LogLevel mMinLevel;
        std::mutex mMutex;
    };

}
```

### MFTScanner - 고속 파일 스캔

#### adapters/platform/win32/storage/MFTScanner.h
```cpp
#pragma once

#include <Windows.h>
#include <winioctl.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    struct VolumeContents {
        bool hasWindowsDirectory;
        bool hasSystem32Directory;
        bool hasUsersDirectory;
        bool hasProgramFilesDirectory;
        std::unordered_set<std::wstring> topLevelDirectories;
        uint64_t totalFiles;
        uint64_t totalDirectories;
    };

    class MFTScanner {
    public:
        [[nodiscard]] static domain::Expected<VolumeContents> ScanVolume(
            const std::wstring& volumePath
        );

    private:
        struct MFTRecord {
            uint64_t fileReference;
            uint64_t parentFileReference;
            std::wstring fileName;
            bool isDirectory;
        };

        [[nodiscard]] static domain::Expected<std::vector<MFTRecord>> ReadMFT(
            HANDLE hVolume
        );

        static VolumeContents AnalyzeMFTRecords(
            const std::vector<MFTRecord>& records
        );

        static constexpr size_t BUFFER_SIZE = 64 * 1024;
        static constexpr uint32_t MFT_RECORD_SIZE = 1024;
    };

}
```

---

## 🏗️ Visual Studio 프로젝트 구성

### 솔루션 구조

```
WinSetup.sln
├── WinSetup (실행 프로젝트)
│   ├── 플랫폼: x64
│   ├── 구성: Debug, Release
│   ├── 출력 형식: Application (.exe)
│   ├── C++ 표준: C++20 (/std:c++20)
│   ├── 문자 집합: Unicode
│   └── 링크 라이브러리:
│       ├── wimlib.lib
│       ├── DismApi.lib
│       ├── setupapi.lib
│       └── ntdll.lib
│
└── WinSetup.Tests (테스트 프로젝트)
    ├── 플랫폼: x64
    ├── 구성: Debug, Release
    ├── 출력 형식: Application (.exe)
    ├── 링크 라이브러리:
    │   ├── gtest.lib
    │   ├── gtest_main.lib
    │   └── gmock.lib
    └── 테스트 프레임워크: Google Test
```

### WinSetup.vcxproj 주요 설정

```xml
<PropertyGroup>
  <ConfigurationType>Application</ConfigurationType>
  <PlatformToolset>v143</PlatformToolset>
  <CharacterSet>Unicode</CharacterSet>
  <LanguageStandard>stdcpp20</LanguageStandard>
</PropertyGroup>

<ItemDefinitionGroup>
  <ClCompile>
    <WarningLevel>Level4</WarningLevel>
    <TreatWarningAsError>true</TreatWarningAsError>
    <ConformanceMode>true</ConformanceMode>
    <MultiProcessorCompilation>true</MultiProcessorCompilation>
    <PreprocessorDefinitions>
      _UNICODE;
      UNICODE;
      WIN32_LEAN_AND_MEAN;
      NOMINMAX;
      _WIN32_WINNT=0x0A00;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <AdditionalIncludeDirectories>
      $(ProjectDir)src;
      $(ProjectDir)lib;
      %(AdditionalIncludeDirectories)
    </AdditionalIncludeDirectories>
  </ClCompile>
  
  <Link>
    <SubSystem>Windows</SubSystem>
    <AdditionalLibraryDirectories>
      $(ProjectDir)lib;
      %(AdditionalLibraryDirectories)
    </AdditionalLibraryDirectories>
    <AdditionalDependencies>
      wimlib.lib;
      DismApi.lib;
      setupapi.lib;
      ntdll.lib;
      %(AdditionalDependencies)
    </AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

### 빌드 구성

#### Debug 구성
```xml
<PropertyGroup Condition="'$(Configuration)'=='Debug'">
  <LinkIncremental>true</LinkIncremental>
  <GenerateDebugInformation>true</GenerateDebugInformation>
</PropertyGroup>

<ItemDefinitionGroup Condition="'$(Configuration)'=='Debug'">
  <ClCompile>
    <Optimization>Disabled</Optimization>
    <PreprocessorDefinitions>
      _DEBUG;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
  </ClCompile>
</ItemDefinitionGroup>
```

#### Release 구성
```xml
<PropertyGroup Condition="'$(Configuration)'=='Release'">
  <LinkIncremental>false</LinkIncremental>
  <GenerateDebugInformation>false</GenerateDebugInformation>
</PropertyGroup>

<ItemDefinitionGroup Condition="'$(Configuration)'=='Release'">
  <ClCompile>
    <Optimization>MaxSpeed</Optimization>
    <FunctionLevelLinking>true</FunctionLevelLinking>
    <IntrinsicFunctions>true</IntrinsicFunctions>
    <PreprocessorDefinitions>
      NDEBUG;
      %(PreprocessorDefinitions)
    </PreprocessorDefinitions>
    <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
  </ClCompile>
  
  <Link>
    <EnableCOMDATFolding>true</EnableCOMDATFolding>
    <OptimizeReferences>true</OptimizeReferences>
  </Link>
</ItemDefinitionGroup>
```

### 필터 구조 (.vcxproj.filters)

```xml
<ItemGroup>
  <Filter Include="src">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\domain">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\domain\primitives">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\abstractions">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\application">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\adapters">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\adapters\platform\win32">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
  <Filter Include="src\main">
    <UniqueIdentifier>{...}</UniqueIdentifier>
  </Filter>
</ItemGroup>
```

---

## 📅 구현 단계 (Phase별)

### Phase 1: 기초 인프라 (1주)

**목표**: 완전히 독립적인 기본 타입 구현

**구현 순서**:
1. **domain/primitives/Error** (1일)
   - ErrorCategory enum
   - Error 클래스
   - ErrorContext 구조체
   - ToString() 구현

2. **domain/primitives/Expected** (2일)
   - Expected<T> 템플릿
   - Expected<void> 특수화
   - Monadic 연산 (Map, FlatMap, UnwrapOr)
   - 테스트 작성

3. **domain/memory/PoolAllocator** (1일)
   - 메모리 풀 구현
   - 스레드 안전성 보장
   - 테스트 작성

4. **abstractions/infrastructure/logging** (2일)
   - LogLevel enum
   - ILogger 인터페이스
   - 기본 로깅 구조 정의

**완료 기준**:
- ✅ Expected<T> 단위 테스트 통과
- ✅ PoolAllocator 스레드 안전성 테스트 통과
- ✅ 모든 코드 컴파일 성공

### Phase 2: Domain Entities (1주)

**목표**: 비즈니스 엔티티 및 값 객체 구현

**구현 순서**:
1. **domain/valueobjects** (2일)
   - BusType, DiskType enums
   - DiskSize 클래스
   - FileSystemType enum
   - PartitionType enum
   - DriveLetter 클래스

2. **domain/entities** (3일)
   - DiskInfo
   - PartitionInfo
   - VolumeInfo
   - SystemInfo
   - SetupConfig

3. **domain/services** (2일)
   - DiskSortingService
   - PartitionAnalyzer
   - PathNormalizer

**완료 기준**:
- ✅ 모든 엔티티 불변성 보장
- ✅ 엔티티 단위 테스트 작성
- ✅ 도메인 서비스 로직 검증

### Phase 3: Win32 Core Infrastructure (1주)

**목표**: Win32 플랫폼 추상화 계층

**구현 순서**:
1. **adapters/platform/win32/memory** (1일)
   - UniqueHandle
   - UniqueLibrary
   - UniqueFindHandle

2. **adapters/platform/win32/core** (2일)
   - Win32TypeMapper
   - Win32ErrorHandler
   - Win32StringHelper
   - Win32HandleFactory
   - Win32Constants

3. **adapters/platform/win32/logging** (2일)
   - Win32Logger 구현
   - 버퍼링 전략
   - Flush 정책
   - 파일 로테이션

4. **통합 테스트** (2일)
   - RAII 래퍼 안전성 검증
   - 로거 성능 테스트
   - 메모리 누수 검사

**완료 기준**:
- ✅ UniqueHandle 리소스 자동 해제 검증
- ✅ Win32Logger 버퍼링 동작 확인
- ✅ 메모리 누수 없음

### Phase 4: Application Layer (1주)

**목표**: Use Cases 및 DI Container

**구현 순서**:
1. **application/core** (2일)
   - DIContainer 구현
   - ServiceLocator
   - 의존성 주입 테스트

2. **application/async** (1일)
   - Task<T> 코루틴
   - CancellationToken
   - Promise/Awaitable

3. **application/usecases/system** (2일)
   - AnalyzeSystemUseCase
   - LoadConfigurationUseCase

4. **application/services** (2일)
   - EventBus
   - Dispatcher
   - TaskScheduler

**완료 기준**:
- ✅ DIContainer 순환 의존성 검증
- ✅ 코루틴 동작 테스트
- ✅ EventBus 동시성 테스트

### Phase 5: Storage Adapters (2주)

**목표**: 디스크/볼륨 서비스 구현

**구현 순서**:
1. **IOCTLWrapper + AsyncIOCTL** (3일)
   - IOCTL_DISK_* 래핑
   - OVERLAPPED 비동기 I/O
   - 에러 처리

2. **Win32DiskService** (3일)
   - EnumerateDisks
   - GetDiskInfo
   - CleanDisk
   - CreatePartitionLayout
   - FormatPartition

3. **Win32VolumeService** (2일)
   - EnumerateVolumes
   - GetVolumeInfo
   - MountVolume / DismountVolume

4. **MFTScanner** (2일)
   - FSCTL_GET_NTFS_VOLUME_DATA
   - MFT 직접 읽기
   - 파티션 내용 분석

5. **DiskTransaction** (2일)
   - Step 기반 트랜잭션
   - Rollback 구현
   - 레이아웃 백업/복원

**완료 기준**:
- ✅ 실제 디스크 열거 성공
- ✅ MFT 스캔 < 1초
- ✅ 트랜잭션 롤백 검증

### Phase 6: SMBIOS & System Info (1주)

**목표**: 시스템 정보 수집

**구현 순서**:
1. **SMBIOSStructures** (1일)
   - SMBIOS 구조체 정의

2. **SMBIOSParser** (2일)
   - GetSystemFirmwareTable
   - 파싱 로직

3. **Win32SystemInfoService** (2일)
   - GetMotherboardModel
   - GetBIOSVersion
   - IsUEFIBoot
   - GetTotalMemoryBytes

4. **통합 테스트** (2일)

**완료 기준**:
- ✅ SMBIOS 정보 정확히 파싱
- ✅ UEFI/Legacy 판별 정확도

### Phase 7: Imaging Services (2주)

**목표**: WIM 이미지 처리

**구현 순서**:
1. **WimlibAdapter** (4일)
   - wimlib 초기화
   - ApplyImage
   - CaptureImage
   - GetImageInfo

2. **WimlibOptimizer** (2일)
   - 압축 레벨 조정
   - 멀티스레드 설정
   - 메모리 제한

3. **DismAdapter** (2일)
   - DismApi 초기화
   - 드라이버 주입

4. **ProgressCallback** (1일)
   - 진행률 보고
   - 이벤트 발행

5. **통합 테스트** (3일)
   - 실제 WIM 파일 테스트
   - 성능 벤치마크

**완료 기준**:
- ✅ WIM 적용 성공
- ✅ 진행률 정확도
- ✅ Config.ini 예상 시간 준수

### Phase 8: Use Cases (1주)

**목표**: 비즈니스 로직 완성

**구현 순서**:
1. **usecases/disk** (2일)
   - EnumerateDisksUseCase
   - AnalyzeDisksUseCase
   - SelectTargetDisksUseCase

2. **usecases/install** (3일)
   - InstallWindowsUseCase
   - BackupUserDataUseCase
   - RestoreUserDataUseCase
   - InjectDriversUseCase

3. **통합 테스트** (2일)
   - End-to-End 시나리오
   - 에러 처리 검증

**완료 기준**:
- ✅ 전체 플로우 동작
- ✅ 롤백 시나리오 검증
- ✅ 모든 Use Case 테스트 통과

### Phase 9: UI Layer (1주)

**목표**: Win32 UI 구현

**구현 순서**:
1. **controls** (2일)
   - SimpleButton
   - ToggleButton

2. **Win32MainWindow** (3일)
   - 윈도우 생성
   - 메시지 루프
   - 이벤트 핸들링

3. **Win32ProgressBar** (1일)
   - 진행률 표시

4. **UI 통합** (1일)

**완료 기준**:
- ✅ UI 렌더링 정상
- ✅ 사용자 입력 처리
- ✅ 진행률 업데이트

### Phase 10: Composition Root (3일)

**목표**: 모든 것을 연결

**구현 순서**:
1. **ServiceRegistration** (1일)
   - 모든 서비스 등록
   - 의존성 와이어링

2. **Main.cpp** (1일)
   - 애플리케이션 초기화
   - 메인 플로우

3. **최종 통합 테스트** (1일)

**완료 기준**:
- ✅ WinPE 환경에서 실행
- ✅ 전체 설치 프로세스 완료
- ✅ 메모리 사용량 < 512MB

---

## 🎯 성공 기준

### 기능 요구사항
- ✅ 디스크 열거 및 분석
- ✅ 파티션 레이아웃 생성 (UEFI/Legacy)
- ✅ WIM 이미지 적용
- ✅ 드라이버 주입
- ✅ 부트로더 구성
- ✅ 사용자 데이터 백업/복원

### 비기능 요구사항
- ✅ 성능: 디스크 열거 < 3초, MFT 스캔 < 1초
- ✅ 메모리: < 512MB
- ✅ 안정성: 트랜잭션 롤백 100% 성공
- ✅ 테스트: 코드 커버리지 > 80%
- ✅ 코드 품질: 모든 경고 제거

### 아키텍처 준수
- ✅ Domain 계층 완전 격리 (Windows.h 없음)
- ✅ 의존성 규칙 위반 0건
- ✅ 모든 리소스 RAII 관리
- ✅ Expected<T> 일관된 사용

---

## 📚 참고 문서

### 필수 읽기
- `ARCHITECTURE.md` - 아키텍처 상세 설명
- `IMPLEMENTATION_GUIDE.md` - 구현 가이드라인
- `API_REFERENCE.md` - API 문서
- `PERFORMANCE_TUNING.md` - 성능 최적화 가이드

### 외부 자료
- [wimlib 문서](https://wimlib.net/documentation.html)
- [MSDN: IOCTL_DISK_*](https://docs.microsoft.com/en-us/windows/win32/api/winioctl/)
- [SMBIOS Specification](https://www.dmtf.org/standards/smbios)
- [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)

---

**계획서 작성 완료일**: 2026년 2월 15일  
**버전**: 1.0  
**작성자**: 시니어 개발자  
**상태**: 최종 승인 대기
