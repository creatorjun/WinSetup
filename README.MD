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
    auto hDisk = adapters::platform::UniqueHandle(...);
    // 자동으로 CloseHandle 호출
}

// ✅ 모든 Win32 리소스에 적용
adapters::platform::UniqueHandle hFile;
adapters::platform::UniqueLibrary hModule;
adapters::platform::UniqueFindHandle hFind;
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
    .Map([](const auto& disk) { return disk.GetSize(); })
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
│   │   │   ├── persistence/                  # ✅ 실제 구현에 존재
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
│   │   │   └── UniqueHandleTests.cpp
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

    private:
        bool mHasValue;
        Error mError;
    };

}
```

### Domain Events - 독립적인 구조

#### domain/events/DomainEvent.h
```cpp
#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <atomic>

namespace winsetup::domain {

    class DomainEvent {
    public:
        virtual ~DomainEvent() = default;

        [[nodiscard]] uint64_t GetEventId() const noexcept { return m_eventId; }
        [[nodiscard]] auto GetTimestamp() const noexcept { return m_timestamp; }
        [[nodiscard]] virtual std::wstring GetEventType() const noexcept = 0;

    protected:
        DomainEvent()
            : m_eventId(GenerateEventId())
            , m_timestamp(std::chrono::system_clock::now())
        {}

    private:
        static uint64_t GenerateEventId() noexcept {
            static std::atomic<uint64_t> counter{ 0 };
            return counter.fetch_add(1, std::memory_order_relaxed);
        }

        uint64_t m_eventId;
        std::chrono::system_clock::time_point m_timestamp;
    };

}
```

**중요**: Domain Events는 `abstractions::IEvent`와는 별개의 독립적인 도메인 이벤트 시스템입니다. 도메인 계층에서 발생하는 비즈니스 이벤트를 표현하며, 정적 카운터를 통한 자동 ID 생성 기능을 포함합니다.

---

## 📡 Abstractions 계층 설계 (순수 인터페이스)

### ILogger - 로깅 인터페이스

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

        // ⚠️ 주의: Flush() 메서드는 인터페이스에 없음
        // Win32Logger 구현 클래스에만 존재하는 추가 기능
    };

}
```

**변경 사항**: `Flush()` 메서드가 인터페이스에서 제거되었습니다. 이는 구현 세부사항이므로 `Win32Logger` 클래스에만 존재합니다.
