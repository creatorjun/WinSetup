# WinSetup 클린 아키텍처 + 저수준 최적화 구현 계획서 v2.0

## 📋 목차

**Part 1 (현재 문서):**
1. [개요](#개요)
2. [아키텍처 원칙](#아키텍처-원칙)
3. [계층 구조](#계층-구조)
4. [폴더 구조](#폴더-구조)
5. [Domain 계층 설계](#domain-계층-설계)
6. [Abstractions 계층 설계](#abstractions-계층-설계)

**Part 2:**
7. Application 계층 설계
8. Adapters 계층 저수준 구현
9. 멀티스레딩 및 성능 최적화
10. 구체적 실행 로직
11. 구현 가이드

---

## 개요

### 목표
Windows PE 환경에서 실행되는 PC 초기화 프로그램을 **완벽한 클린 아키텍처**와 **저수준 고성능 API**를 결합하여 구현합니다.

### 핵심 설계 철학
1. **클린 아키텍처**: Domain 계층의 완전한 격리
2. **고성능**: IOCP, IOCTL, wimlib 등 저수준 API 활용
3. **비동기 처리**: Task Dependency Graph 기반 병렬 실행
4. **플랫폼 독립성**: Domain/Application 계층은 플랫폼 중립적

### 기술 스택
- **언어**: C++20/23 (코루틴 활용)
- **플랫폼**: Windows PE (Windows 10/11)
- **저수준 API**: 
  - IOCTL (디스크 제어)
  - IOCP (비동기 I/O)
  - GetSystemFirmwareTable (SMBIOS)
  - wimlib (이미지 처리)
  - DismApi (드라이버 주입)
- **빌드**: MSVC 2022
- **테스트**: Google Test

---

## 아키텍처 원칙

### 1. 완전한 계층 격리

```
┌─────────────────────────────────────────┐
│  Domain (Layer 1)                       │
│  ✓ 외부 의존성 0                         │
│  ✓ 표준 C++만 사용                       │
│  ✓ 플랫폼 타입 금지                      │
│  ✗ Windows.h 절대 금지                  │
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
│  ✓ 비즈니스 플로우                       │
│  ✗ 플랫폼 코드 금지                      │
└─────────────────────────────────────────┘
            ↑
            │ (인터페이스 구현)
┌─────────────────────────────────────────┐
│  Adapters (Layer 3)                     │
│  ✓ IOCTL, IOCP 구현                     │
│  ✓ wimlib, SMBIOS 파싱                  │
│  ✓ 플랫폼 타입 변환                      │
└─────────────────────────────────────────┘
            ↑
            │ (모든 것 연결)
┌─────────────────────────────────────────┐
│  Main (Layer 4)                         │
│  ✓ Composition Root                     │
│  ✓ DI Container                         │
└─────────────────────────────────────────┘
```

### 2. 플랫폼 타입 변환 원칙

```cpp
// ❌ 절대 금지: Domain에 Windows 타입
namespace domain {
    struct DiskInfo {
        STORAGE_BUS_TYPE busType;  // Windows SDK 타입
        HANDLE handle;             // Windows 핸들
    };
}

// ✅ 필수: Domain 자체 타입
namespace domain {
    enum class BusType : uint8_t {
        Unknown = 0,
        SCSI = 1,
        ATAPI = 2,
        ATA = 3,
        USB = 7,
        SATA = 11,
        SAS = 10,
        NVMe = 17,
        VirtualDisk = 18
    };
    
    struct DiskInfo {
        uint32_t index;
        BusType busType;           // ✅ 플랫폼 독립적
        uint64_t sizeBytes;
        std::wstring devicePath;
    };
}

// ✅ Adapter에서 변환
namespace adapters::win32 {
    domain::BusType MapBusType(STORAGE_BUS_TYPE win32Type) {
        switch (win32Type) {
            case BusTypeNvme: return domain::BusType::NVMe;
            case BusTypeSata: return domain::BusType::SATA;
            case BusTypeUsb:  return domain::BusType::USB;
            default:          return domain::BusType::Unknown;
        }
    }
}
```

### 3. 성능과 추상화의 균형

```cpp
// 추상화: 테스트 가능성
class IDiskService {
    virtual Expected<std::vector<DiskInfo>> EnumerateDisks() = 0;
};

// 구현: 최고 성능
class Win32DiskService : public IDiskService {
    Expected<std::vector<DiskInfo>> EnumerateDisks() override {
        // IOCP 기반 병렬 열거
        // IOCTL_STORAGE_QUERY_PROPERTY 직접 호출
        // 0-copy 데이터 변환
    }
};
```

---

## 계층 구조

### 전체 의존성 다이어그램

```
                    Main.cpp
                       │
                       │ (모든 것 연결)
                       ▼
    ┌──────────────────────────────────────┐
    │     Dependency Container             │
    │  - Service Registration              │
    │  - Lifetime Management               │
    └──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
   ┌─────────┐  ┌──────────┐  ┌─────────┐
   │  Use    │  │ Event    │  │   UI    │
   │  Cases  │  │ Handlers │  │ Windows │
   └─────────┘  └──────────┘  └─────────┘
        │              │              │
        └──────────────┼──────────────┘
                       │ (인터페이스 의존)
                       ▼
    ┌──────────────────────────────────────┐
    │      Abstractions (Interfaces)       │
    │  - IRepository, IService             │
    │  - IEventBus, ILogger                │
    └──────────────────────────────────────┘
                       │
                       │ (Domain 타입 참조)
                       ▼
    ┌──────────────────────────────────────┐
    │         Domain (Core Logic)          │
    │  - Entities, Value Objects           │
    │  - Domain Services                   │
    │  - Specifications, Events            │
    └──────────────────────────────────────┘
```

### 계층별 책임

#### Layer 1: Domain
**책임:**
- 비즈니스 규칙 구현
- 엔티티 정의
- 도메인 이벤트 정의
- 플랫폼 독립적 로직

**예시:**
```cpp
// 디스크 정렬 규칙
class DiskSortingService {
    static std::vector<DiskInfo> SortByPriority(
        const std::vector<DiskInfo>& disks
    ) {
        // 순수 비즈니스 로직: BusType 우선, Size 오름차순
    }
};

// 파티션 역할 판별 규칙
class PartitionAnalyzer {
    static VolumeRole AnalyzeRole(
        const VolumeInfo& volume,
        const std::wstring& userProfile
    ) {
        // 순수 논리: 디렉터리 존재 패턴 분석
    }
};
```

#### Layer 0: Abstractions
**책임:**
- 인터페이스 정의
- 계약 명시

**예시:**
```cpp
class IDiskService {
    virtual Expected<std::vector<DiskInfo>> EnumerateDisks() = 0;
    virtual Expected<void> CleanDisk(uint32_t index) = 0;
    virtual Expected<void> CreatePartitionLayout(
        uint32_t index,
        const PartitionLayout& layout
    ) = 0;
};
```

#### Layer 2: Application
**책임:**
- Use Case 구현
- 플로우 조율
- 이벤트 발행/구독

**예시:**
```cpp
class AnalyzeDisksUseCase {
    Expected<DiskAnalysisResult> Execute() {
        // 1. 디스크 열거 (IDiskService 사용)
        // 2. 정렬 (Domain Service 호출)
        // 3. 분석 (Domain Service 호출)
        // 4. 이벤트 발행 (IEventBus 사용)
    }
};
```

#### Layer 3: Adapters
**책임:**
- 저수준 API 호출
- 플랫폼 타입 변환
- 고성능 구현

**예시:**
```cpp
class Win32DiskService : public IDiskService {
    Expected<std::vector<DiskInfo>> EnumerateDisks() override {
        // SetupDiGetClassDevs
        // DeviceIoControl(IOCTL_STORAGE_QUERY_PROPERTY)
        // 병렬 처리 (IOCP)
        // 타입 변환: STORAGE_BUS_TYPE -> domain::BusType
    }
};
```

---

## 폴더 구조

```
WinSetup/
│
├── README.md
├── ARCHITECTURE.md
│
├── src/
│   │
│   ├── domain/                          # Layer 1: 완전 격리
│   │   │
│   │   ├── entities/
│   │   │   ├── DiskInfo.h
│   │   │   ├── DiskInfo.cpp
│   │   │   ├── VolumeInfo.h
│   │   │   ├── VolumeInfo.cpp
│   │   │   ├── PartitionInfo.h
│   │   │   ├── PartitionInfo.cpp
│   │   │   ├── SystemInfo.h
│   │   │   ├── SystemInfo.cpp
│   │   │   ├── SetupConfig.h
│   │   │   └── SetupConfig.cpp
│   │   │
│   │   ├── valueobjects/
│   │   │   ├── BusType.h
│   │   │   ├── DiskSize.h
│   │   │   ├── DiskSize.cpp
│   │   │   ├── PartitionType.h
│   │   │   ├── FileSystemType.h
│   │   │   ├── DriveLetter.h
│   │   │   └── DriveLetter.cpp
│   │   │
│   │   ├── services/
│   │   │   ├── DiskSortingService.h
│   │   │   ├── DiskSortingService.cpp
│   │   │   ├── PartitionAnalyzer.h
│   │   │   ├── PartitionAnalyzer.cpp
│   │   │   ├── PathNormalizer.h
│   │   │   └── PathNormalizer.cpp
│   │   │
│   │   ├── specifications/
│   │   │   ├── ISpecification.h
│   │   │   ├── DiskSpecifications.h
│   │   │   ├── DiskSpecifications.cpp
│   │   │   ├── VolumeSpecifications.h
│   │   │   └── VolumeSpecifications.cpp
│   │   │
│   │   ├── events/
│   │   │   ├── DomainEvent.h
│   │   │   ├── DomainEvent.cpp
│   │   │   ├── DiskAnalyzedEvent.h
│   │   │   ├── InstallStartedEvent.h
│   │   │   ├── InstallProgressEvent.h
│   │   │   ├── InstallCompletedEvent.h
│   │   │   └── ErrorOccurredEvent.h
│   │   │
│   │   ├── primitives/
│   │   │   ├── Expected.h
│   │   │   ├── Result.h
│   │   │   └── Error.h
│   │   │
│   │   ├── functional/
│   │   │   ├── Monads.h
│   │   │   ├── Optional.h
│   │   │   ├── Pipeline.h
│   │   │   └── Compose.h
│   │   │
│   │   └── memory/
│   │       ├── SmartPtr.h
│   │       ├── UniqueResource.h
│   │       ├── SharedResource.h
│   │       └── PoolAllocator.h
│   │
│   ├── abstractions/                    # Layer 0: 순수 인터페이스
│   │   │
│   │   ├── repositories/
│   │   │   ├── IConfigRepository.h
│   │   │   ├── IDiskRepository.h
│   │   │   └── IVolumeRepository.h
│   │   │
│   │   ├── services/
│   │   │   ├── platform/
│   │   │   │   ├── ITextEncoder.h
│   │   │   │   ├── ISystemInfoService.h
│   │   │   │   └── IThreadPool.h
│   │   │   │
│   │   │   └── storage/
│   │   │       ├── IDiskService.h
│   │   │       ├── IVolumeService.h
│   │   │       ├── IPartitionService.h
│   │   │       ├── IStorageScanner.h
│   │   │       ├── IImagingService.h
│   │   │       └── IDriverService.h
│   │   │
│   │   ├── infrastructure/
│   │   │   ├── async/
│   │   │   │   ├── IExecutor.h
│   │   │   │   ├── IScheduler.h
│   │   │   │   └── IAsyncContext.h
│   │   │   │
│   │   │   ├── messaging/
│   │   │   │   ├── IEvent.h
│   │   │   │   ├── IEventBus.h
│   │   │   │   ├── IDispatcher.h
│   │   │   │   └── IMessageQueue.h
│   │   │   │
│   │   │   └── logging/
│   │   │       ├── ILogger.h
│   │   │       └── LogLevel.h
│   │   │
│   │   └── ui/
│   │       ├── IWindow.h
│   │       ├── IWidget.h
│   │       └── IProgressBar.h
│   │
│   ├── application/                     # Layer 2: Use Cases
│   │   ├── usecases/
│   │   │   ├── system/
│   │   │   │   ├── AnalyzeSystemUseCase.h
│   │   │   │   ├── AnalyzeSystemUseCase.cpp
│   │   │   │   ├── LoadConfigurationUseCase.h
│   │   │   │   └── LoadConfigurationUseCase.cpp
│   │   │   │
│   │   │   ├── disk/
│   │   │   │   ├── EnumerateDisksUseCase.h
│   │   │   │   ├── EnumerateDisksUseCase.cpp
│   │   │   │   ├── AnalyzeDisksUseCase.h
│   │   │   │   ├── AnalyzeDisksUseCase.cpp
│   │   │   │   ├── SelectTargetDisksUseCase.h
│   │   │   │   └── SelectTargetDisksUseCase.cpp
│   │   │   │
│   │   │   └── install/
│   │   │       ├── InstallWindowsUseCase.h
│   │   │       ├── InstallWindowsUseCase.cpp
│   │   │       ├── BackupUserDataUseCase.h
│   │   │       ├── BackupUserDataUseCase.cpp
│   │   │       ├── RestoreUserDataUseCase.h
│   │   │       ├── RestoreUserDataUseCase.cpp
│   │   │       ├── ApplyImageUseCase.h
│   │   │       ├── ApplyImageUseCase.cpp
│   │   │       ├── InjectDriversUseCase.h
│   │   │       └── InjectDriversUseCase.cpp
│   │   │
│   │   ├── services/
│   │   │   ├── async/
│   │   │   │   ├── Task.h
│   │   │   │   ├── TaskScheduler.h
│   │   │   │   ├── TaskScheduler.cpp
│   │   │   │   ├── AsyncContext.h
│   │   │   │   ├── AsyncContext.cpp
│   │   │   │   ├── CancellationToken.h
│   │   │   │   └── CancellationToken.cpp
│   │   │   │
│   │   │   ├── messaging/
│   │   │   │   ├── EventBus.h
│   │   │   │   ├── EventBus.cpp
│   │   │   │   ├── Dispatcher.h
│   │   │   │   ├── Dispatcher.cpp
│   │   │   │   └── MessageQueue.h
│   │   │   │
│   │   │   └── reactive/
│   │   │       ├── Property.h
│   │   │       ├── Observable.h
│   │   │       └── Subject.h
│   │   │
│   │   ├── eventhandlers/
│   │   │   ├── DiskAnalyzedEventHandler.h
│   │   │   ├── DiskAnalyzedEventHandler.cpp
│   │   │   ├── InstallProgressEventHandler.h
│   │   │   └── InstallProgressEventHandler.cpp
│   │   │
│   │   └── dto/
│   │       ├── SystemAnalysisResult.h
│   │       ├── DiskAnalysisResult.h
│   │       └── InstallationProgress.h
│   │
│   ├── adapters/                        # Layer 3: 저수준 구현
│   │   │
│   │   ├── platform/
│   │   │   └── win32/
│   │   │       ├── core/
│   │   │       │   ├── Win32TypeMapper.h
│   │   │       │   ├── Win32TypeMapper.cpp
│   │   │       │   ├── Win32ErrorHandler.h
│   │   │       │   └── Win32ErrorHandler.cpp
│   │   │       │
│   │   │       ├── encoding/
│   │   │       │   ├── Win32TextEncoder.h
│   │   │       │   └── Win32TextEncoder.cpp
│   │   │       │
│   │   │       ├── system/
│   │   │       │   ├── Win32SystemInfoService.h
│   │   │       │   ├── Win32SystemInfoService.cpp
│   │   │       │   ├── SMBIOSParser.h
│   │   │       │   ├── SMBIOSParser.cpp
│   │   │       │   ├── SMBIOSStructures.h
│   │   │       │   └── FirmwareTableReader.h
│   │   │       │
│   │   │       ├── storage/
│   │   │       │   ├── Win32DiskService.h
│   │   │       │   ├── Win32DiskService.cpp
│   │   │       │   ├── Win32VolumeService.h
│   │   │       │   ├── Win32VolumeService.cpp
│   │   │       │   ├── Win32PartitionService.h
│   │   │       │   ├── Win32PartitionService.cpp
│   │   │       │   ├── Win32StorageScanner.h
│   │   │       │   ├── Win32StorageScanner.cpp
│   │   │       │   ├── IOCTLWrapper.h
│   │   │       │   ├── IOCTLWrapper.cpp
│   │   │       │   ├── DiskLayoutBuilder.h
│   │   │       │   └── DiskLayoutBuilder.cpp
│   │   │       │
│   │   │       ├── threading/
│   │   │       │   ├── Win32ThreadPool.h
│   │   │       │   ├── Win32ThreadPool.cpp
│   │   │       │   ├── IOCompletionPort.h
│   │   │       │   ├── IOCompletionPort.cpp
│   │   │       │   ├── Win32Thread.h
│   │   │       │   └── Win32Thread.cpp
│   │   │       │
│   │   │       └── Win32PlatformFactory.h
│   │   │
│   │   ├── persistence/
│   │   │   ├── config/
│   │   │   │   ├── IniConfigRepository.h
│   │   │   │   ├── IniConfigRepository.cpp
│   │   │   │   ├── IniParser.h
│   │   │   │   └── IniParser.cpp
│   │   │   │
│   │   │   └── filesystem/
│   │   │       ├── Win32FileSystem.h
│   │   │       ├── Win32FileSystem.cpp
│   │   │       ├── Win32File.h
│   │   │       └── Win32File.cpp
│   │   │
│   │   ├── imaging/
│   │   │   ├── WimlibAdapter.h
│   │   │   ├── WimlibAdapter.cpp
│   │   │   ├── WimlibProgressCallback.h
│   │   │   ├── WimlibProgressCallback.cpp
│   │   │   ├── DismAdapter.h
│   │   │   └── DismAdapter.cpp
│   │   │
│   │   └── ui/
│   │       └── win32/
│   │           ├── Win32MainWindow.h
│   │           ├── Win32MainWindow.cpp
│   │           ├── Win32StatusWidget.h
│   │           ├── Win32StatusWidget.cpp
│   │           ├── Win32ProgressBar.h
│   │           ├── Win32ProgressBar.cpp
│   │           └── Win32UIFactory.h
│   │
│   ├── main/                            # Layer 4: Composition Root
│   │   ├── Main.cpp
│   │   ├── DIContainer.h
│   │   ├── DIContainer.cpp
│   │   ├── ServiceRegistration.h
│   │   └── ServiceRegistration.cpp
│   │
│   └── resources/
│       ├── resource.h
│       └── app.manifest
│
├── tests/
│   ├── domain/
│   ├── application/
│   ├── adapters/
│   ├── integration/
│   └── mocks/
│
├── docs/
│   ├── architecture/
│   ├── api-reference/
│   └── guides/
│
└── scripts/
    ├── build.ps1
    ├── test.ps1
    └── analyze.ps1
```

---

## Domain 계층 설계

### Entities (엔티티)

#### DiskInfo.h
```cpp
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include "../valueobjects/BusType.h"
#include "../valueobjects/DiskSize.h"

namespace winsetup::domain {

    class DiskInfo {
    public:
        DiskInfo(
            uint32_t index,
            BusType busType,
            DiskSize size,
            std::wstring devicePath
        );

        [[nodiscard]] uint32_t GetIndex() const noexcept;
        [[nodiscard]] BusType GetBusType() const noexcept;
        [[nodiscard]] DiskSize GetSize() const noexcept;
        [[nodiscard]] const std::wstring& GetDevicePath() const noexcept;

        [[nodiscard]] bool IsRemovable() const noexcept;
        [[nodiscard]] bool IsSuitableForSystem() const noexcept;
        [[nodiscard]] int GetPriority() const noexcept;

        [[nodiscard]] bool operator==(const DiskInfo& other) const noexcept;
        [[nodiscard]] bool operator!=(const DiskInfo& other) const noexcept;

    private:
        uint32_t m_index;
        BusType m_busType;
        DiskSize m_size;
        std::wstring m_devicePath;
    };

}
```

#### VolumeInfo.h
```cpp
#pragma once

#include <string>
#include <optional>
#include "../valueobjects/DriveLetter.h"
#include "../valueobjects/FileSystemType.h"

namespace winsetup::domain {

    enum class VolumeRole {
        Unknown,
        System,
        Data,
        Boot,
        Recovery
    };

    class VolumeInfo {
    public:
        VolumeInfo(
            std::optional<DriveLetter> letter,
            FileSystemType fileSystem,
            uint64_t totalBytes,
            uint64_t freeBytes,
            std::wstring volumePath
        );

        [[nodiscard]] std::optional<DriveLetter> GetDriveLetter() const noexcept;
        [[nodiscard]] FileSystemType GetFileSystem() const noexcept;
        [[nodiscard]] uint64_t GetTotalBytes() const noexcept;
        [[nodiscard]] uint64_t GetFreeBytes() const noexcept;
        [[nodiscard]] const std::wstring& GetVolumePath() const noexcept;

        [[nodiscard]] double GetUsagePercentage() const noexcept;
        [[nodiscard]] bool HasEnoughSpace(uint64_t requiredBytes) const noexcept;

        void SetRole(VolumeRole role) noexcept;
        [[nodiscard]] VolumeRole GetRole() const noexcept;

    private:
        std::optional<DriveLetter> m_letter;
        FileSystemType m_fileSystem;
        uint64_t m_totalBytes;
        uint64_t m_freeBytes;
        std::wstring m_volumePath;
        VolumeRole m_role;
    };

}
```

#### SetupConfig.h
```cpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace winsetup::domain {

    struct SetupType {
        std::wstring name;
        std::wstring description;
        std::wstring infoText;

        [[nodiscard]] bool IsValid() const noexcept {
            return !name.empty();
        }
    };

    struct BackupPath {
        std::wstring name;
        std::wstring pathTemplate;

        [[nodiscard]] std::wstring Resolve(
            const std::wstring& userProfile
        ) const;
    };

    class SetupConfig {
    public:
        SetupConfig(
            std::wstring userProfile,
            std::vector<BackupPath> backupPaths,
            std::vector<SetupType> types,
            std::map<std::wstring, std::chrono::seconds> estimatedTimes
        );

        [[nodiscard]] const std::wstring& GetUserProfile() const noexcept;
        [[nodiscard]] const std::vector<BackupPath>& GetBackupPaths() const noexcept;
        [[nodiscard]] const std::vector<SetupType>& GetTypes() const noexcept;

        [[nodiscard]] std::chrono::seconds GetEstimatedTime(
            const std::wstring& motherboardModel
        ) const noexcept;

        [[nodiscard]] std::optional<SetupType> GetTypeByName(
            const std::wstring& name
        ) const noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

    private:
        std::wstring m_userProfile;
        std::vector<BackupPath> m_backupPaths;
        std::vector<SetupType> m_types;
        std::map<std::wstring, std::chrono::seconds> m_estimatedTimes;
    };

}
```

### Value Objects (값 객체)

#### BusType.h
```cpp
#pragma once

#include <cstdint>
#include <string>

namespace winsetup::domain {

    enum class BusType : uint8_t {
        Unknown = 0,
        SCSI = 1,
        ATAPI = 2,
        ATA = 3,
        IEEE1394 = 4,
        SSA = 5,
        FibreChannel = 6,
        USB = 7,
        RAID = 8,
        iSCSI = 9,
        SAS = 10,
        SATA = 11,
        SD = 12,
        MMC = 13,
        NVMe = 17,
        VirtualDisk = 18,
        FileBackedVirtual = 19
    };

    constexpr int GetBusTypePriority(BusType type) noexcept {
        switch (type) {
            case BusType::NVMe:          return 100;
            case BusType::SAS:           return 90;
            case BusType::SATA:          return 80;
            case BusType::RAID:          return 70;
            case BusType::SCSI:          return 60;
            case BusType::FibreChannel:  return 50;
            case BusType::iSCSI:         return 40;
            case BusType::ATA:           return 30;
            case BusType::USB:           return 10;
            default:                     return 0;
        }
    }

    constexpr bool IsRemovable(BusType type) noexcept {
        return type == BusType::USB ||
               type == BusType::SD ||
               type == BusType::MMC;
    }

    constexpr bool IsFastStorage(BusType type) noexcept {
        return type == BusType::NVMe ||
               type == BusType::SAS ||
               type == BusType::SATA;
    }

    constexpr const wchar_t* BusTypeToString(BusType type) noexcept {
        switch (type) {
            case BusType::NVMe:          return L"NVMe";
            case BusType::SATA:          return L"SATA";
            case BusType::SAS:           return L"SAS";
            case BusType::USB:           return L"USB";
            case BusType::RAID:          return L"RAID";
            case BusType::VirtualDisk:   return L"Virtual";
            default:                     return L"Unknown";
        }
    }

}
```

#### DiskSize.h
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <compare>

namespace winsetup::domain {

    class DiskSize {
    public:
        explicit constexpr DiskSize(uint64_t bytes = 0) noexcept
            : m_bytes(bytes) {}

        [[nodiscard]] constexpr uint64_t GetBytes() const noexcept {
            return m_bytes;
        }

        [[nodiscard]] constexpr double GetKB() const noexcept {
            return static_cast<double>(m_bytes) / KB;
        }

        [[nodiscard]] constexpr double GetMB() const noexcept {
            return static_cast<double>(m_bytes) / MB;
        }

        [[nodiscard]] constexpr double GetGB() const noexcept {
            return static_cast<double>(m_bytes) / GB;
        }

        [[nodiscard]] constexpr double GetTB() const noexcept {
            return static_cast<double>(m_bytes) / TB;
        }

        [[nodiscard]] std::wstring ToString() const;

        [[nodiscard]] auto operator<=>(const DiskSize&) const = default;

        static constexpr uint64_t KB = 1024ULL;
        static constexpr uint64_t MB = 1024ULL * KB;
        static constexpr uint64_t GB = 1024ULL * MB;
        static constexpr uint64_t TB = 1024ULL * GB;

    private:
        uint64_t m_bytes;
    };

    inline constexpr DiskSize operator"" _KB(unsigned long long kb) {
        return DiskSize(kb * DiskSize::KB);
    }

    inline constexpr DiskSize operator"" _MB(unsigned long long mb) {
        return DiskSize(mb * DiskSize::MB);
    }

    inline constexpr DiskSize operator"" _GB(unsigned long long gb) {
        return DiskSize(gb * DiskSize::GB);
    }

    inline constexpr DiskSize operator"" _TB(unsigned long long tb) {
        return DiskSize(tb * DiskSize::TB);
    }

}
```

### Domain Services

#### DiskSortingService.h
```cpp
#pragma once

#include <vector>
#include <algorithm>
#include "../entities/DiskInfo.h"
#include "../specifications/DiskSpecifications.h"

namespace winsetup::domain {

    class DiskSortingService {
    public:
        struct SortedDisks {
            std::vector<DiskInfo> eligible;
            std::vector<DiskInfo> excluded;
        };

        [[nodiscard]] static SortedDisks FilterAndSort(
            const std::vector<DiskInfo>& disks
        );

        [[nodiscard]] static std::vector<DiskInfo> SortByPriority(
            const std::vector<DiskInfo>& disks
        );

    private:
        static constexpr bool ShouldExcludeDisk(const DiskInfo& disk) noexcept {
            return disk.IsRemovable();
        }

        static constexpr int CalculatePriority(const DiskInfo& disk) noexcept {
            int priority = GetBusTypePriority(disk.GetBusType()) * 1000;
            
            priority -= static_cast<int>(disk.GetSize().GetGB());
            
            return priority;
        }
    };

}
```

#### PartitionAnalyzer.h
```cpp
#pragma once

#include <vector>
#include <string>
#include "../entities/VolumeInfo.h"

namespace winsetup::domain {

    struct VolumeAnalysisResult {
        VolumeInfo volume;
        VolumeRole suggestedRole;
        int confidenceScore;

        [[nodiscard]] bool IsConfident() const noexcept {
            return confidenceScore >= 80;
        }
    };

    class PartitionAnalyzer {
    public:
        struct AnalysisContext {
            std::wstring userProfile;
            std::vector<std::wstring> systemMarkers;
            std::vector<std::wstring> dataMarkers;

            static AnalysisContext CreateDefault(
                const std::wstring& userProfile
            );
        };

        [[nodiscard]] static VolumeAnalysisResult AnalyzeVolume(
            const VolumeInfo& volume,
            const AnalysisContext& context
        );

        [[nodiscard]] static std::vector<VolumeAnalysisResult> AnalyzeAll(
            const std::vector<VolumeInfo>& volumes,
            const AnalysisContext& context
        );

    private:
        static int CalculateSystemScore(
            const VolumeInfo& volume,
            const AnalysisContext& context
        );

        static int CalculateDataScore(
            const VolumeInfo& volume,
            const AnalysisContext& context
        );
    };

}
```

### Specifications

#### ISpecification.h
```cpp
#pragma once

#include <memory>
#include <concepts>

namespace winsetup::domain {

    template<typename T>
    class ISpecification {
    public:
        virtual ~ISpecification() = default;

        [[nodiscard]] virtual bool IsSatisfiedBy(const T& candidate) const = 0;

        [[nodiscard]] std::unique_ptr<ISpecification<T>> And(
            std::unique_ptr<ISpecification<T>> other
        ) const;

        [[nodiscard]] std::unique_ptr<ISpecification<T>> Or(
            std::unique_ptr<ISpecification<T>> other
        ) const;

        [[nodiscard]] std::unique_ptr<ISpecification<T>> Not() const;
    };

}
```

#### DiskSpecifications.h
```cpp
#pragma once

#include "ISpecification.h"
#include "../entities/DiskInfo.h"
#include "../valueobjects/BusType.h"
#include "../valueobjects/DiskSize.h"

namespace winsetup::domain {

    class NonRemovableDiskSpec : public ISpecification<DiskInfo> {
    public:
        [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return !disk.IsRemovable();
        }
    };

    class MinimumSizeSpec : public ISpecification<DiskInfo> {
    public:
        explicit MinimumSizeSpec(DiskSize minimumSize)
            : m_minimumSize(minimumSize) {}

        [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.GetSize() >= m_minimumSize;
        }

    private:
        DiskSize m_minimumSize;
    };

    class FastStorageSpec : public ISpecification<DiskInfo> {
    public:
        [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return IsFastStorage(disk.GetBusType());
        }
    };

    class SystemDiskSpec : public ISpecification<DiskInfo> {
    public:
        [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return NonRemovableDiskSpec().IsSatisfiedBy(disk) &&
                   MinimumSizeSpec(128_GB).IsSatisfiedBy(disk) &&
                   FastStorageSpec().IsSatisfiedBy(disk);
        }
    };

}
```

### Domain Events

#### DomainEvent.h
```cpp
#pragma once

#include <chrono>
#include <typeindex>

namespace winsetup::domain {

    class DomainEvent {
    public:
        DomainEvent()
            : m_occurredAt(std::chrono::system_clock::now()) {}

        virtual ~DomainEvent() = default;

        [[nodiscard]] auto GetOccurredAt() const noexcept {
            return m_occurredAt;
        }

        [[nodiscard]] virtual std::type_index GetType() const noexcept = 0;

    private:
        std::chrono::system_clock::time_point m_occurredAt;
    };

}
```

#### DiskAnalyzedEvent.h
```cpp
#pragma once

#include "DomainEvent.h"
#include "../entities/DiskInfo.h"
#include <vector>

namespace winsetup::domain {

    class DiskAnalyzedEvent : public DomainEvent {
    public:
        DiskAnalyzedEvent(
            std::vector<DiskInfo> allDisks,
            std::vector<DiskInfo> eligibleDisks,
            DiskInfo selectedSystemDisk,
            DiskInfo selectedDataDisk
        )
            : m_allDisks(std::move(allDisks))
            , m_eligibleDisks(std::move(eligibleDisks))
            , m_selectedSystemDisk(std::move(selectedSystemDisk))
            , m_selectedDataDisk(std::move(selectedDataDisk))
        {}

        [[nodiscard]] const auto& GetAllDisks() const noexcept {
            return m_allDisks;
        }

        [[nodiscard]] const auto& GetEligibleDisks() const noexcept {
            return m_eligibleDisks;
        }

        [[nodiscard]] const auto& GetSelectedSystemDisk() const noexcept {
            return m_selectedSystemDisk;
        }

        [[nodiscard]] const auto& GetSelectedDataDisk() const noexcept {
            return m_selectedDataDisk;
        }

        [[nodiscard]] std::type_index GetType() const noexcept override {
            return typeid(DiskAnalyzedEvent);
        }

    private:
        std::vector<DiskInfo> m_allDisks;
        std::vector<DiskInfo> m_eligibleDisks;
        DiskInfo m_selectedSystemDisk;
        DiskInfo m_selectedDataDisk;
    };

}
```

---

## Abstractions 계층 설계

### Repository Interfaces

#### IConfigRepository.h
```cpp
#pragma once

#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/SetupConfig.h"

namespace winsetup::abstractions {

    class IConfigRepository {
    public:
        virtual ~IConfigRepository() = default;

        [[nodiscard]] virtual domain::Expected<domain::SetupConfig> 
            Load(const std::wstring& path) = 0;

        [[nodiscard]] virtual domain::Expected<void> 
            Save(const domain::SetupConfig& config, const std::wstring& path) = 0;

        [[nodiscard]] virtual domain::Expected<bool> 
            Exists(const std::wstring& path) = 0;
    };

}
```

### Service Interfaces - Storage

#### IDiskService.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include "../../../domain/entities/DiskInfo.h"
#include "../../../domain/entities/PartitionInfo.h"
#include <vector>

namespace winsetup::abstractions {

    struct PartitionLayout {
        enum class Style {
            MBR,
            GPT
        };

        Style style;
        std::vector<domain::PartitionInfo> partitions;
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
    };

}
```

#### IImagingService.h
```cpp
#pragma once

#include "../../../domain/primitives/Expected.h"
#include <string>
#include <functional>

namespace winsetup::abstractions {

    struct ImageProgress {
        uint64_t totalBytes;
        uint64_t completedBytes;
        double percentage;
        std::wstring currentFile;
    };

    using ProgressCallback = std::function<void(const ImageProgress&)>;

    class IImagingService {
    public:
        virtual ~IImagingService() = default;

        [[nodiscard]] virtual domain::Expected<void> 
            ApplyImage(
                const std::wstring& wimPath,
                int imageIndex,
                const std::wstring& targetPath,
                ProgressCallback progressCallback = nullptr
            ) = 0;

        [[nodiscard]] virtual domain::Expected<int> 
            GetImageCount(const std::wstring& wimPath) = 0;

        [[nodiscard]] virtual domain::Expected<std::wstring> 
            GetImageName(const std::wstring& wimPath, int imageIndex) = 0;
    };

}
```

### Infrastructure Interfaces

#### IEvent.h
```cpp
#pragma once

#include <typeindex>
#include <memory>
#include <concepts>

namespace winsetup::abstractions {

    class IEvent {
    public:
        virtual ~IEvent() = default;

        [[nodiscard]] virtual std::type_index GetType() const noexcept = 0;
    };

    template<typename T>
    concept EventType = std::is_base_of_v<IEvent, T> && 
                        requires(T t) {
                            { t.GetType() } -> std::same_as<std::type_index>;
                        };

}
```

#### IEventBus.h
```cpp
#pragma once

#include "IEvent.h"
#include <memory>
#include <functional>
#include <typeindex>

namespace winsetup::abstractions {

    using SubscriptionToken = size_t;

    class IEventBus {
    public:
        virtual ~IEventBus() = default;

        virtual void Publish(std::shared_ptr<IEvent> event) = 0;

        virtual SubscriptionToken Subscribe(
            std::type_index eventType,
            std::function<void(std::shared_ptr<IEvent>)> handler
        ) = 0;

        template<EventType TEvent>
        SubscriptionToken Subscribe(
            std::function<void(std::shared_ptr<TEvent>)> handler
        ) {
            return Subscribe(
                typeid(TEvent),
                [handler](std::shared_ptr<IEvent> event) {
                    auto typedEvent = std::static_pointer_cast<TEvent>(event);
                    handler(typedEvent);
                }
            );
        }

        virtual void Unsubscribe(SubscriptionToken token) = 0;
        virtual void Clear() = 0;

        [[nodiscard]] virtual size_t GetSubscriberCount() const = 0;
    };

}
```

---
# WinSetup 클린 아키텍처 + 저수준 최적화 구현 계획서 v2.0 (Part 2)

---

## Application 계층 설계

### Use Cases

#### AnalyzeSystemUseCase.h
```cpp
#pragma once

#include "../../abstractions/services/platform/ISystemInfoService.h"
#include "../../abstractions/repositories/IConfigRepository.h"
#include "../../abstractions/infrastructure/logging/ILogger.h"
#include "../../abstractions/infrastructure/messaging/IEventBus.h"
#include "../../domain/primitives/Expected.h"
#include "../dto/SystemAnalysisResult.h"
#include <memory>

namespace winsetup::application {

    class AnalyzeSystemUseCase {
    public:
        AnalyzeSystemUseCase(
            std::shared_ptr<abstractions::ISystemInfoService> systemInfo,
            std::shared_ptr<abstractions::IConfigRepository> configRepo,
            std::shared_ptr<abstractions::IEventBus> eventBus,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<SystemAnalysisResult> Execute(
            const std::wstring& configPath
        );

    private:
        std::shared_ptr<abstractions::ISystemInfoService> m_systemInfo;
        std::shared_ptr<abstractions::IConfigRepository> m_configRepo;
        std::shared_ptr<abstractions::IEventBus> m_eventBus;
        std::shared_ptr<abstractions::ILogger> m_logger;

        domain::Expected<std::wstring> GetMotherboardModel();
        domain::Expected<domain::SetupConfig> LoadConfiguration(
            const std::wstring& path
        );
        SystemAnalysisResult BuildResult(
            const std::wstring& motherboardModel,
            const domain::SetupConfig& config
        );
    };

}
```

#### EnumerateDisksUseCase.h
```cpp
#pragma once

#include "../../abstractions/services/storage/IDiskService.h"
#include "../../abstractions/infrastructure/async/IExecutor.h"
#include "../../abstractions/infrastructure/logging/ILogger.h"
#include "../../domain/primitives/Expected.h"
#include "../../domain/entities/DiskInfo.h"
#include <memory>
#include <vector>

namespace winsetup::application {

    class EnumerateDisksUseCase {
    public:
        EnumerateDisksUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IExecutor> executor,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<std::vector<domain::DiskInfo>> Execute();

    private:
        std::shared_ptr<abstractions::IDiskService> m_diskService;
        std::shared_ptr<abstractions::IExecutor> m_executor;
        std::shared_ptr<abstractions::ILogger> m_logger;
    };

}
```

#### AnalyzeDisksUseCase.h
```cpp
#pragma once

#include "../../abstractions/services/storage/IDiskService.h"
#include "../../abstractions/services/storage/IStorageScanner.h"
#include "../../abstractions/infrastructure/messaging/IEventBus.h"
#include "../../abstractions/infrastructure/logging/ILogger.h"
#include "../../domain/primitives/Expected.h"
#include "../../domain/services/DiskSortingService.h"
#include "../../domain/services/PartitionAnalyzer.h"
#include "../dto/DiskAnalysisResult.h"
#include <memory>

namespace winsetup::application {

    class AnalyzeDisksUseCase {
    public:
        AnalyzeDisksUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IStorageScanner> scanner,
            std::shared_ptr<abstractions::IEventBus> eventBus,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<DiskAnalysisResult> Execute(
            const std::wstring& userProfile
        );

    private:
        std::shared_ptr<abstractions::IDiskService> m_diskService;
        std::shared_ptr<abstractions::IStorageScanner> m_scanner;
        std::shared_ptr<abstractions::IEventBus> m_eventBus;
        std::shared_ptr<abstractions::ILogger> m_logger;

        domain::Expected<std::vector<domain::DiskInfo>> EnumerateAndFilter();
        domain::Expected<DiskAnalysisResult> SelectTargetDisks(
            const std::vector<domain::DiskInfo>& sortedDisks,
            const std::wstring& userProfile
        );
        void PublishDiskAnalyzedEvent(const DiskAnalysisResult& result);
    };

}
```

#### InstallWindowsUseCase.h
```cpp
#pragma once

#include "../../abstractions/services/storage/IDiskService.h"
#include "../../abstractions/services/storage/IVolumeService.h"
#include "../../abstractions/services/storage/IImagingService.h"
#include "../../abstractions/services/storage/IDriverService.h"
#include "../../abstractions/infrastructure/messaging/IEventBus.h"
#include "../../abstractions/infrastructure/logging/ILogger.h"
#include "../../domain/primitives/Expected.h"
#include "../dto/InstallationProgress.h"
#include <memory>

namespace winsetup::application {

    enum class InstallMode {
        PreserveData,
        CleanInstall
    };

    struct InstallOptions {
        InstallMode mode;
        domain::DiskInfo systemDisk;
        domain::DiskInfo dataDisk;
        std::wstring wimPath;
        int imageIndex;
        std::wstring userProfile;
        std::vector<std::wstring> backupPaths;
    };

    class InstallWindowsUseCase {
    public:
        InstallWindowsUseCase(
            std::shared_ptr<abstractions::IDiskService> diskService,
            std::shared_ptr<abstractions::IVolumeService> volumeService,
            std::shared_ptr<abstractions::IImagingService> imagingService,
            std::shared_ptr<abstractions::IDriverService> driverService,
            std::shared_ptr<abstractions::IEventBus> eventBus,
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<void> Execute(
            const InstallOptions& options
        );

    private:
        std::shared_ptr<abstractions::IDiskService> m_diskService;
        std::shared_ptr<abstractions::IVolumeService> m_volumeService;
        std::shared_ptr<abstractions::IImagingService> m_imagingService;
        std::shared_ptr<abstractions::IDriverService> m_driverService;
        std::shared_ptr<abstractions::IEventBus> m_eventBus;
        std::shared_ptr<abstractions::ILogger> m_logger;

        domain::Expected<void> ExecutePreserveMode(
            const InstallOptions& options
        );

        domain::Expected<void> ExecuteCleanMode(
            const InstallOptions& options
        );

        domain::Expected<void> BackupUserData(
            const std::wstring& sourcePath,
            const std::wstring& targetPath
        );

        domain::Expected<void> RestoreUserData(
            const std::wstring& sourcePath,
            const std::wstring& targetPath
        );

        domain::Expected<void> ApplyWindowsImage(
            const std::wstring& wimPath,
            int imageIndex,
            const std::wstring& targetPath
        );

        domain::Expected<void> InjectDrivers(
            const std::wstring& targetPath,
            const std::wstring& driversPath
        );

        void PublishProgress(const InstallationProgress& progress);
    };

}
```

### DTO (Data Transfer Objects)

#### SystemAnalysisResult.h
```cpp
#pragma once

#include <string>
#include <chrono>
#include "../../domain/entities/SetupConfig.h"

namespace winsetup::application {

    struct SystemAnalysisResult {
        std::wstring motherboardModel;
        std::wstring biosVersion;
        domain::SetupConfig config;
        std::chrono::seconds estimatedInstallTime;
        
        [[nodiscard]] bool IsValid() const noexcept {
            return !motherboardModel.empty() && config.IsValid();
        }
    };

}
```

#### DiskAnalysisResult.h
```cpp
#pragma once

#include <vector>
#include "../../domain/entities/DiskInfo.h"
#include "../../domain/entities/VolumeInfo.h"

namespace winsetup::application {

    struct DiskAnalysisResult {
        std::vector<domain::DiskInfo> allDisks;
        std::vector<domain::DiskInfo> eligibleDisks;
        domain::DiskInfo selectedSystemDisk;
        domain::DiskInfo selectedDataDisk;
        std::vector<domain::VolumeInfo> systemVolumes;
        std::vector<domain::VolumeInfo> dataVolumes;
        
        [[nodiscard]] bool HasValidSelection() const noexcept {
            return selectedSystemDisk.GetIndex() != UINT32_MAX;
        }
    };

}
```

---

## Adapters 계층 저수준 구현

### Win32 Type Mapper

#### Win32TypeMapper.h
```cpp
#pragma once

#include <Windows.h>
#include <winioctl.h>
#include "../../../../domain/valueobjects/BusType.h"
#include "../../../../domain/valueobjects/PartitionType.h"
#include "../../../../domain/valueobjects/FileSystemType.h"

namespace winsetup::adapters::win32 {

    class Win32TypeMapper {
    public:
        [[nodiscard]] static domain::BusType MapBusType(
            STORAGE_BUS_TYPE win32Type
        ) noexcept;

        [[nodiscard]] static STORAGE_BUS_TYPE MapBusType(
            domain::BusType domainType
        ) noexcept;

        [[nodiscard]] static domain::PartitionType MapPartitionType(
            PARTITION_STYLE style,
            BYTE partitionType
        ) noexcept;

        [[nodiscard]] static domain::FileSystemType MapFileSystemType(
            const std::wstring& fileSystemName
        ) noexcept;

        [[nodiscard]] static PARTITION_STYLE MapPartitionStyle(
            domain::PartitionType type
        ) noexcept;
    };

}
```

#### Win32TypeMapper.cpp
```cpp
#include "Win32TypeMapper.h"

namespace winsetup::adapters::win32 {

    domain::BusType Win32TypeMapper::MapBusType(
        STORAGE_BUS_TYPE win32Type
    ) noexcept {
        switch (win32Type) {
            case BusTypeScsi:           return domain::BusType::SCSI;
            case BusTypeAtapi:          return domain::BusType::ATAPI;
            case BusTypeAta:            return domain::BusType::ATA;
            case BusType1394:           return domain::BusType::IEEE1394;
            case BusTypeSsa:            return domain::BusType::SSA;
            case BusTypeFibre:          return domain::BusType::FibreChannel;
            case BusTypeUsb:            return domain::BusType::USB;
            case BusTypeRAID:           return domain::BusType::RAID;
            case BusTypeiScsi:          return domain::BusType::iSCSI;
            case BusTypeSas:            return domain::BusType::SAS;
            case BusTypeSata:           return domain::BusType::SATA;
            case BusTypeSd:             return domain::BusType::SD;
            case BusTypeMmc:            return domain::BusType::MMC;
            case BusTypeNvme:           return domain::BusType::NVMe;
            case BusTypeVirtual:        return domain::BusType::VirtualDisk;
            case BusTypeFileBackedVirtual: return domain::BusType::FileBackedVirtual;
            default:                    return domain::BusType::Unknown;
        }
    }

    STORAGE_BUS_TYPE Win32TypeMapper::MapBusType(
        domain::BusType domainType
    ) noexcept {
        switch (domainType) {
            case domain::BusType::SCSI:             return BusTypeScsi;
            case domain::BusType::ATAPI:            return BusTypeAtapi;
            case domain::BusType::ATA:              return BusTypeAta;
            case domain::BusType::USB:              return BusTypeUsb;
            case domain::BusType::SATA:             return BusTypeSata;
            case domain::BusType::SAS:              return BusTypeSas;
            case domain::BusType::NVMe:             return BusTypeNvme;
            case domain::BusType::VirtualDisk:      return BusTypeVirtual;
            default:                                return BusTypeUnknown;
        }
    }

}
```

### IOCTL Wrapper (저수준 디스크 제어)

#### IOCTLWrapper.h
```cpp
#pragma once

#include <Windows.h>
#include <winioctl.h>
#include <memory>
#include <vector>
#include "../../../../domain/primitives/Expected.h"
#include "../../../../domain/primitives/Error.h"

namespace winsetup::adapters::win32 {

    class IOCTLWrapper {
    public:
        struct DiskGeometry {
            uint64_t cylinders;
            uint32_t tracksPerCylinder;
            uint32_t sectorsPerTrack;
            uint32_t bytesPerSector;
            uint64_t totalSize;
        };

        struct StorageDeviceInfo {
            STORAGE_BUS_TYPE busType;
            std::string vendorId;
            std::string productId;
            std::string serialNumber;
            bool removable;
        };

        [[nodiscard]] static domain::Expected<DiskGeometry> GetDiskGeometry(
            HANDLE hDisk
        ) noexcept;

        [[nodiscard]] static domain::Expected<StorageDeviceInfo> QueryStorageProperty(
            HANDLE hDisk
        ) noexcept;

        [[nodiscard]] static domain::Expected<DRIVE_LAYOUT_INFORMATION_EX> GetDriveLayout(
            HANDLE hDisk
        ) noexcept;

        [[nodiscard]] static domain::Expected<void> SetDriveLayout(
            HANDLE hDisk,
            const DRIVE_LAYOUT_INFORMATION_EX& layout
        ) noexcept;

        [[nodiscard]] static domain::Expected<void> CleanDisk(
            HANDLE hDisk
        ) noexcept;

        [[nodiscard]] static domain::Expected<void> CreateDisk(
            HANDLE hDisk,
            PARTITION_STYLE style
        ) noexcept;

        [[nodiscard]] static domain::Expected<void> UpdateDiskProperties(
            HANDLE hDisk
        ) noexcept;

    private:
        static constexpr DWORD DEFAULT_TIMEOUT_MS = 30000;

        template<typename TInput, typename TOutput>
        [[nodiscard]] static domain::Expected<TOutput> DeviceIoControlWrapper(
            HANDLE hDevice,
            DWORD ioControlCode,
            const TInput* input = nullptr,
            DWORD inputSize = 0,
            DWORD outputBufferSize = sizeof(TOutput)
        ) noexcept;
    };

}
```

#### IOCTLWrapper.cpp (핵심 구현)
```cpp
#include "IOCTLWrapper.h"
#include <memory>

namespace winsetup::adapters::win32 {

    domain::Expected<IOCTLWrapper::StorageDeviceInfo> 
    IOCTLWrapper::QueryStorageProperty(HANDLE hDisk) noexcept {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        constexpr DWORD bufferSize = 4096;
        auto buffer = std::make_unique<BYTE[]>(bufferSize);

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            buffer.get(),
            bufferSize,
            &bytesReturned,
            nullptr
        );

        if (!result) {
            return domain::Error{
                L"IOCTL_STORAGE_QUERY_PROPERTY failed",
                GetLastError()
            };
        }

        auto descriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.get());

        StorageDeviceInfo info{};
        info.busType = descriptor->BusType;
        info.removable = descriptor->RemovableMedia;

        if (descriptor->VendorIdOffset > 0) {
            info.vendorId = reinterpret_cast<char*>(buffer.get() + descriptor->VendorIdOffset);
        }

        if (descriptor->ProductIdOffset > 0) {
            info.productId = reinterpret_cast<char*>(buffer.get() + descriptor->ProductIdOffset);
        }

        if (descriptor->SerialNumberOffset > 0) {
            info.serialNumber = reinterpret_cast<char*>(buffer.get() + descriptor->SerialNumberOffset);
        }

        return info;
    }

    domain::Expected<void> IOCTLWrapper::CleanDisk(HANDLE hDisk) noexcept {
        CREATE_DISK createDisk{};
        createDisk.PartitionStyle = PARTITION_STYLE_RAW;

        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk,
            IOCTL_DISK_CREATE_DISK,
            &createDisk,
            sizeof(createDisk),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        if (!result) {
            return domain::Error{
                L"IOCTL_DISK_CREATE_DISK failed",
                GetLastError()
            };
        }

        return {};
    }

    domain::Expected<void> IOCTLWrapper::SetDriveLayout(
        HANDLE hDisk,
        const DRIVE_LAYOUT_INFORMATION_EX& layout
    ) noexcept {
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(
            hDisk,
            IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            const_cast<DRIVE_LAYOUT_INFORMATION_EX*>(&layout),
            sizeof(layout),
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        if (!result) {
            return domain::Error{
                L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX failed",
                GetLastError()
            };
        }

        result = DeviceIoControl(
            hDisk,
            IOCTL_DISK_UPDATE_PROPERTIES,
            nullptr,
            0,
            nullptr,
            0,
            &bytesReturned,
            nullptr
        );

        if (!result) {
            return domain::Error{
                L"IOCTL_DISK_UPDATE_PROPERTIES failed",
                GetLastError()
            };
        }

        return {};
    }

}
```

### Win32DiskService (IOCTL 기반 디스크 제어)

#### Win32DiskService.h
```cpp
#pragma once

#include "../../../../abstractions/services/storage/IDiskService.h"
#include "../../../../abstractions/infrastructure/logging/ILogger.h"
#include "IOCTLWrapper.h"
#include "Win32TypeMapper.h"
#include <memory>

namespace winsetup::adapters::win32 {

    class Win32DiskService : public abstractions::IDiskService {
    public:
        explicit Win32DiskService(
            std::shared_ptr<abstractions::ILogger> logger
        );

        [[nodiscard]] domain::Expected<std::vector<domain::DiskInfo>> 
            EnumerateDisks() override;

        [[nodiscard]] domain::Expected<domain::DiskInfo> 
            GetDiskInfo(uint32_t diskIndex) override;

        [[nodiscard]] domain::Expected<void> 
            CleanDisk(uint32_t diskIndex) override;

        [[nodiscard]] domain::Expected<void> 
            CreatePartitionLayout(
                uint32_t diskIndex,
                const abstractions::PartitionLayout& layout
            ) override;

        [[nodiscard]] domain::Expected<void> 
            FormatPartition(
                uint32_t diskIndex,
                uint32_t partitionIndex,
                domain::FileSystemType fileSystem,
                bool quickFormat
            ) override;

    private:
        std::shared_ptr<abstractions::ILogger> m_logger;

        [[nodiscard]] domain::Expected<HANDLE> OpenDisk(
            uint32_t diskIndex,
            DWORD accessMode = GENERIC_READ | GENERIC_WRITE
        ) noexcept;

        [[nodiscard]] domain::Expected<domain::DiskInfo> QueryDiskInfo(
            uint32_t diskIndex,
            HANDLE hDisk
        ) noexcept;

        [[nodiscard]] domain::Expected<DRIVE_LAYOUT_INFORMATION_EX> BuildDriveLayout(
            const abstractions::PartitionLayout& layout,
            uint64_t diskSize
        ) noexcept;
    };

}
```

#### Win32DiskService.cpp (병렬 열거 구현)
```cpp
#include "Win32DiskService.h"
#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <thread>
#include <future>
#include <vector>

namespace winsetup::adapters::win32 {

    domain::Expected<std::vector<domain::DiskInfo>> 
    Win32DiskService::EnumerateDisks() {
        m_logger->Info(L"Starting parallel disk enumeration");

        std::vector<std::future<domain::Expected<domain::DiskInfo>>> futures;

        for (uint32_t index = 0; index < 32; ++index) {
            futures.push_back(
                std::async(std::launch::async, [this, index]() {
                    return GetDiskInfo(index);
                })
            );
        }

        std::vector<domain::DiskInfo> disks;
        for (auto& future : futures) {
            auto result = future.get();
            if (result.HasValue()) {
                disks.push_back(std::move(result.Value()));
            }
        }

        m_logger->Info(L"Found " + std::to_wstring(disks.size()) + L" disks");
        return disks;
    }

    domain::Expected<domain::DiskInfo> 
    Win32DiskService::GetDiskInfo(uint32_t diskIndex) {
        auto handleResult = OpenDisk(diskIndex, GENERIC_READ);
        if (!handleResult.HasValue()) {
            return handleResult.Error();
        }

        HANDLE hDisk = handleResult.Value();
        auto closeGuard = [hDisk]() { CloseHandle(hDisk); };
        std::unique_ptr<void, decltype(closeGuard)> guard(hDisk, closeGuard);

        return QueryDiskInfo(diskIndex, hDisk);
    }

    domain::Expected<HANDLE> Win32DiskService::OpenDisk(
        uint32_t diskIndex,
        DWORD accessMode
    ) noexcept {
        std::wstring path = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);

        HANDLE hDisk = CreateFileW(
            path.c_str(),
            accessMode,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hDisk == INVALID_HANDLE_VALUE) {
            return domain::Error{
                L"Failed to open disk " + std::to_wstring(diskIndex),
                GetLastError()
            };
        }

        return hDisk;
    }

    domain::Expected<domain::DiskInfo> Win32DiskService::QueryDiskInfo(
        uint32_t diskIndex,
        HANDLE hDisk
    ) noexcept {
        auto storageInfo = IOCTLWrapper::QueryStorageProperty(hDisk);
        if (!storageInfo.HasValue()) {
            return storageInfo.Error();
        }

        auto geometry = IOCTLWrapper::GetDiskGeometry(hDisk);
        if (!geometry.HasValue()) {
            return geometry.Error();
        }

        domain::BusType busType = Win32TypeMapper::MapBusType(
            storageInfo.Value().busType
        );

        std::wstring devicePath = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);

        return domain::DiskInfo(
            diskIndex,
            busType,
            domain::DiskSize(geometry.Value().totalSize),
            devicePath
        );
    }

    domain::Expected<void> Win32DiskService::CleanDisk(uint32_t diskIndex) {
        m_logger->Info(L"Cleaning disk " + std::to_wstring(diskIndex));

        auto handleResult = OpenDisk(diskIndex);
        if (!handleResult.HasValue()) {
            return handleResult.Error();
        }

        HANDLE hDisk = handleResult.Value();
        auto closeGuard = [hDisk]() { CloseHandle(hDisk); };
        std::unique_ptr<void, decltype(closeGuard)> guard(hDisk, closeGuard);

        return IOCTLWrapper::CleanDisk(hDisk);
    }

    domain::Expected<void> Win32DiskService::CreatePartitionLayout(
        uint32_t diskIndex,
        const abstractions::PartitionLayout& layout
    ) {
        m_logger->Info(L"Creating partition layout on disk " + std::to_wstring(diskIndex));

        auto handleResult = OpenDisk(diskIndex);
        if (!handleResult.HasValue()) {
            return handleResult.Error();
        }

        HANDLE hDisk = handleResult.Value();
        auto closeGuard = [hDisk]() { CloseHandle(hDisk); };
        std::unique_ptr<void, decltype(closeGuard)> guard(hDisk, closeGuard);

        auto geometry = IOCTLWrapper::GetDiskGeometry(hDisk);
        if (!geometry.HasValue()) {
            return geometry.Error();
        }

        auto layoutEx = BuildDriveLayout(layout, geometry.Value().totalSize);
        if (!layoutEx.HasValue()) {
            return layoutEx.Error();
        }

        return IOCTLWrapper::SetDriveLayout(hDisk, layoutEx.Value());
    }

}
```

### SMBIOS Parser (메인보드 정보 추출)

#### SMBIOSParser.h
```cpp
#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <cstdint>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    struct SMBIOSBaseBoardInfo {
        std::wstring manufacturer;
        std::wstring product;
        std::wstring version;
        std::wstring serialNumber;
    };

    struct SMBIOSSystemInfo {
        std::wstring manufacturer;
        std::wstring productName;
        std::wstring version;
        std::wstring uuid;
    };

    class SMBIOSParser {
    public:
        [[nodiscard]] static domain::Expected<std::vector<uint8_t>> 
            ReadFirmwareTable() noexcept;

        [[nodiscard]] static domain::Expected<SMBIOSBaseBoardInfo> 
            ParseBaseBoardInfo(const std::vector<uint8_t>& data) noexcept;

        [[nodiscard]] static domain::Expected<SMBIOSSystemInfo> 
            ParseSystemInfo(const std::vector<uint8_t>& data) noexcept;

    private:
        struct SMBIOSHeader {
            uint8_t type;
            uint8_t length;
            uint16_t handle;
        };

        [[nodiscard]] static std::wstring GetStringAtIndex(
            const uint8_t* data,
            size_t dataSize,
            uint8_t index
        ) noexcept;

        [[nodiscard]] static const uint8_t* FindStructure(
            const std::vector<uint8_t>& data,
            uint8_t type
        ) noexcept;
    };

}
```

#### SMBIOSParser.cpp
```cpp
#include "SMBIOSParser.h"
#include <sysinfoapi.h>

namespace winsetup::adapters::win32 {

    domain::Expected<std::vector<uint8_t>> 
    SMBIOSParser::ReadFirmwareTable() noexcept {
        constexpr DWORD signature = 'RSMB';

        DWORD bufferSize = GetSystemFirmwareTable(signature, 0, nullptr, 0);
        if (bufferSize == 0) {
            return domain::Error{
                L"GetSystemFirmwareTable failed to get size",
                GetLastError()
            };
        }

        std::vector<uint8_t> buffer(bufferSize);

        DWORD result = GetSystemFirmwareTable(
            signature,
            0,
            buffer.data(),
            bufferSize
        );

        if (result == 0) {
            return domain::Error{
                L"GetSystemFirmwareTable failed to read data",
                GetLastError()
            };
        }

        struct RawSMBIOSData {
            BYTE used20CallingMethod;
            BYTE majorVersion;
            BYTE minorVersion;
            BYTE dmiRevision;
            DWORD length;
        };

        if (buffer.size() < sizeof(RawSMBIOSData)) {
            return domain::Error{L"Invalid SMBIOS data size"};
        }

        auto rawData = reinterpret_cast<RawSMBIOSData*>(buffer.data());
        
        std::vector<uint8_t> smbiosData(
            buffer.begin() + sizeof(RawSMBIOSData),
            buffer.begin() + sizeof(RawSMBIOSData) + rawData->length
        );

        return smbiosData;
    }

    domain::Expected<SMBIOSBaseBoardInfo> 
    SMBIOSParser::ParseBaseBoardInfo(const std::vector<uint8_t>& data) noexcept {
        constexpr uint8_t TYPE_BASEBOARD = 2;

        const uint8_t* structure = FindStructure(data, TYPE_BASEBOARD);
        if (!structure) {
            return domain::Error{L"BaseBoard information not found"};
        }

        struct BaseBoardStructure {
            SMBIOSHeader header;
            uint8_t manufacturer;
            uint8_t product;
            uint8_t version;
            uint8_t serialNumber;
        };

        auto baseBoard = reinterpret_cast<const BaseBoardStructure*>(structure);

        SMBIOSBaseBoardInfo info;
        info.manufacturer = GetStringAtIndex(
            structure,
            data.size() - (structure - data.data()),
            baseBoard->manufacturer
        );
        info.product = GetStringAtIndex(
            structure,
            data.size() - (structure - data.data()),
            baseBoard->product
        );
        info.version = GetStringAtIndex(
            structure,
            data.size() - (structure - data.data()),
            baseBoard->version
        );
        info.serialNumber = GetStringAtIndex(
            structure,
            data.size() - (structure - data.data()),
            baseBoard->serialNumber
        );

        return info;
    }

    const uint8_t* SMBIOSParser::FindStructure(
        const std::vector<uint8_t>& data,
        uint8_t type
    ) noexcept {
        const uint8_t* ptr = data.data();
        const uint8_t* end = data.data() + data.size();

        while (ptr < end) {
            auto header = reinterpret_cast<const SMBIOSHeader*>(ptr);

            if (header->type == type) {
                return ptr;
            }

            if (header->type == 127) {
                break;
            }

            ptr += header->length;

            while (ptr < end - 1 && !(ptr == 0 && ptr == 0)) {[1]
                ptr++;
            }
            ptr += 2;
        }

        return nullptr;
    }

    std::wstring SMBIOSParser::GetStringAtIndex(
        const uint8_t* data,
        size_t dataSize,
        uint8_t index
    ) noexcept {
        if (index == 0) {
            return L"";
        }

        auto header = reinterpret_cast<const SMBIOSHeader*>(data);
        const char* strSection = reinterpret_cast<const char*>(data + header->length);

        for (uint8_t i = 1; i < index; ++i) {
            strSection += strlen(strSection) + 1;
        }

        std::string str(strSection);
        return std::wstring(str.begin(), str.end());
    }

}
```

### Wimlib Adapter (고성능 이미징)

#### WimlibAdapter.h
```cpp
#pragma once

#include "../../../../abstractions/services/storage/IImagingService.h"
#include "../../../../abstractions/infrastructure/logging/ILogger.h"
#include "../../../../abstractions/infrastructure/messaging/IEventBus.h"
#include <memory>
#include <wimlib.h>

namespace winsetup::adapters::imaging {

    class WimlibAdapter : public abstractions::IImagingService {
    public:
        WimlibAdapter(
            std::shared_ptr<abstractions::IEventBus> eventBus,
            std::shared_ptr<abstractions::ILogger> logger
        );

        ~WimlibAdapter() override;

        [[nodiscard]] domain::Expected<void> ApplyImage(
            const std::wstring& wimPath,
            int imageIndex,
            const std::wstring& targetPath,
            abstractions::ProgressCallback progressCallback
        ) override;

        [[nodiscard]] domain::Expected<int> GetImageCount(
            const std::wstring& wimPath
        ) override;

        [[nodiscard]] domain::Expected<std::wstring> GetImageName(
            const std::wstring& wimPath,
            int imageIndex
        ) override;

    private:
        std::shared_ptr<abstractions::IEventBus> m_eventBus;
        std::shared_ptr<abstractions::ILogger> m_logger;
        abstractions::ProgressCallback m_currentCallback;

        static wimlib_progress_func_t ProgressCallbackWrapper;

        [[nodiscard]] int HandleProgress(
            wimlib_progress_msg msg,
            const wimlib_progress_info* info
        );
    };

}
```

#### WimlibAdapter.cpp
```cpp
#include "WimlibAdapter.h"
#include <string>

namespace winsetup::adapters::imaging {

    WimlibAdapter::WimlibAdapter(
        std::shared_ptr<abstractions::IEventBus> eventBus,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : m_eventBus(std::move(eventBus))
        , m_logger(std::move(logger))
    {}

    WimlibAdapter::~WimlibAdapter() {
        wimlib_global_cleanup();
    }

    domain::Expected<void> WimlibAdapter::ApplyImage(
        const std::wstring& wimPath,
        int imageIndex,
        const std::wstring& targetPath,
        abstractions::ProgressCallback progressCallback
    ) {
        m_logger->Info(L"Applying WIM image: " + wimPath);
        m_currentCallback = progressCallback;

        WIMStruct* wim = nullptr;
        int ret = wimlib_open_wim(
            wimPath.c_str(),
            WIMLIB_OPEN_FLAG_CHECK_INTEGRITY,
            &wim
        );

        if (ret != 0) {
            return domain::Error{
                L"Failed to open WIM file",
                static_cast<uint32_t>(ret)
            };
        }

        auto closeGuard = [wim]() { wimlib_free(wim); };
        std::unique_ptr<WIMStruct, decltype(closeGuard)> guard(wim, closeGuard);

        wimlib_register_progress_function(
            wim,
            ProgressCallbackWrapper,
            this
        );

        int extractFlags = 
            WIMLIB_EXTRACT_FLAG_SEQUENTIAL |
            WIMLIB_EXTRACT_FLAG_NO_ACLS |
            WIMLIB_EXTRACT_FLAG_STRICT_ACLS;

        ret = wimlib_extract_image(
            wim,
            imageIndex,
            targetPath.c_str(),
            extractFlags
        );

        if (ret != 0) {
            return domain::Error{
                L"Failed to extract WIM image",
                static_cast<uint32_t>(ret)
            };
        }

        m_logger->Info(L"WIM image applied successfully");
        return {};
    }

    wimlib_progress_func_t WimlibAdapter::ProgressCallbackWrapper(
        wimlib_progress_msg msg,
        const wimlib_progress_info* info,
        void* context
    ) {
        auto adapter = static_cast<WimlibAdapter*>(context);
        return static_cast<wimlib_progress_func_t>(
            adapter->HandleProgress(msg, info)
        );
    }

    int WimlibAdapter::HandleProgress(
        wimlib_progress_msg msg,
        const wimlib_progress_info* info
    ) {
        if (msg == WIMLIB_PROGRESS_MSG_EXTRACT_FILE_STRUCTURE ||
            msg == WIMLIB_PROGRESS_MSG_EXTRACT_STREAMS) {

            if (m_currentCallback) {
                abstractions::ImageProgress progress;
                progress.totalBytes = info->extract.total_bytes;
                progress.completedBytes = info->extract.completed_bytes;
                progress.percentage = (info->extract.completed_bytes * 100.0) /
                                    info->extract.total_bytes;
                progress.currentFile = info->extract.current_file_name;

                m_currentCallback(progress);
            }
        }

        return WIMLIB_PROGRESS_STATUS_CONTINUE;
    }

}
```

---

## 멀티스레딩 및 성능 최적화

### IOCP 기반 스레드 풀

#### IOCompletionPort.h
```cpp
#pragma once

#include <Windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include "../../../../domain/primitives/Expected.h"

namespace winsetup::adapters::win32 {

    enum class WorkerType {
        IO,
        Compute
    };

    class IOCompletionPort {
    public:
        explicit IOCompletionPort(
            size_t ioThreadCount,
            size_t computeThreadCount
        );

        ~IOCompletionPort();

        [[nodiscard]] domain::Expected<void> Start();
        void Stop();

        void PostWork(
            std::function<void()> work,
            WorkerType type = WorkerType::Compute
        );

        [[nodiscard]] size_t GetQueuedWorkCount() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;

    private:
        HANDLE m_ioCompletionPort;
        HANDLE m_computeCompletionPort;
        std::vector<HANDLE> m_ioThreads;
        std::vector<HANDLE> m_computeThreads;
        std::atomic<bool> m_running;
        std::atomic<size_t> m_queuedWork;

        static DWORD WINAPI IOWorkerThread(LPVOID param);
        static DWORD WINAPI ComputeWorkerThread(LPVOID param);

        void ProcessWork(HANDLE completionPort);
    };

}
```

### 비동기 Task 스케줄러

#### TaskScheduler.h
```cpp
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "../../abstractions/platform/IThreadPool.h"
#include "Task.h"
#include "CancellationToken.h"

namespace winsetup::application {

    using TaskId = size_t;

    struct TaskDescriptor {
        TaskId id;
        std::function<void()> work;
        std::vector<TaskId> dependencies;
        abstractions::TaskPriority priority;
        abstractions::TaskType type;
        CancellationToken cancellationToken;
    };

    class TaskScheduler {
    public:
        explicit TaskScheduler(
            std::shared_ptr<abstractions::IThreadPool> threadPool
        );

        TaskId ScheduleTask(const TaskDescriptor& descriptor);

        void ExecuteGraph();

        void CancelTask(TaskId id);
        void CancelAll();

        [[nodiscard]] bool IsCompleted(TaskId id) const;
        [[nodiscard]] size_t GetPendingCount() const;

    private:
        std::shared_ptr<abstractions::IThreadPool> m_threadPool;
        std::unordered_map<TaskId, TaskDescriptor> m_tasks;
        std::unordered_map<TaskId, std::vector<TaskId>> m_dependents;
        std::unordered_set<TaskId> m_completed;
        std::unordered_set<TaskId> m_ready;
        mutable std::mutex m_mutex;
        TaskId m_nextId;

        void BuildDependencyGraph();
        void ExecuteReadyTasks();
        void OnTaskCompleted(TaskId id);
        bool AreDependenciesMet(TaskId id) const;
    };

}
```

---

## 구체적 실행 로직

### Step 1: 시스템 분석 및 설정 로드

```cpp
namespace application::workflow {

    Task<SystemAnalysisResult> InitializeSetup() {
        auto systemInfoService = Container::Resolve<ISystemInfoService>();
        auto configRepo = Container::Resolve<IConfigRepository>();
        auto logger = Container::Resolve<ILogger>();

        logger->Info(L"[STEP 1] Starting system analysis");

        auto mbInfo = co_await systemInfoService->GetMotherboardModel();
        if (!mbInfo.HasValue()) {
            co_return mbInfo.Error();
        }

        logger->Info(L"Motherboard: " + mbInfo.Value());

        auto config = co_await configRepo->Load(L"config.ini");
        if (!config.HasValue()) {
            co_return config.Error();
        }

        auto estimatedTime = config.Value().GetEstimatedTime(mbInfo.Value());

        logger->Info(L"Estimated install time: " + 
                     std::to_wstring(estimatedTime.count()) + L" seconds");

        co_return SystemAnalysisResult{
            mbInfo.Value(),
            config.Value(),
            estimatedTime
        };
    }

}
```

### Step 2: 디스크 열거 및 정렬

```cpp
namespace application::workflow {

    Task<DiskAnalysisResult> AnalyzeDisks(const std::wstring& userProfile) {
        auto diskService = Container::Resolve<IDiskService>();
        auto logger = Container::Resolve<ILogger>();

        logger->Info(L"[STEP 2] Enumerating disks in parallel");

        auto allDisks = co_await diskService->EnumerateDisks();
        if (!allDisks.HasValue()) {
            co_return allDisks.Error();
        }

        logger->Info(L"Found " + std::to_wstring(allDisks.Value().size()) + L" disks");

        auto sortResult = domain::DiskSortingService::FilterAndSort(
            allDisks.Value()
        );

        logger->Info(L"Eligible disks: " + 
                     std::to_wstring(sortResult.eligible.size()));
        logger->Info(L"Excluded disks: " + 
                     std::to_wstring(sortResult.excluded.size()));

        if (sortResult.eligible.size() < 2) {
            co_return domain::Error{L"Insufficient eligible disks"};
        }

        auto systemDisk = sortResult.eligible;
        auto dataDisk = sortResult.eligible;[1]

        logger->Info(L"Selected System Disk: " + systemDisk.GetDevicePath());
        logger->Info(L"Selected Data Disk: " + dataDisk.GetDevicePath());

        co_return DiskAnalysisResult{
            allDisks.Value(),
            sortResult.eligible,
            systemDisk,
            dataDisk
        };
    }

}
```

### Step 3: 파티션 분석 (Heuristic)

```cpp
namespace application::workflow {

    Task<VolumeAnalysisResult> AnalyzeVolumes(
        const DiskAnalysisResult& diskResult,
        const std::wstring& userProfile
    ) {
        auto volumeService = Container::Resolve<IVolumeService>();
        auto logger = Container::Resolve<ILogger>();

        logger->Info(L"[STEP 3] Analyzing volumes with heuristics");

        auto context = domain::PartitionAnalyzer::AnalysisContext::CreateDefault(
            userProfile
        );

        std::vector<domain::VolumeInfo> allVolumes;
        
        for (const auto& disk : diskResult.eligibleDisks) {
            auto volumes = co_await volumeService->GetVolumes(disk.GetIndex());
            if (volumes.HasValue()) {
                allVolumes.insert(
                    allVolumes.end(),
                    volumes.Value().begin(),
                    volumes.Value().end()
                );
            }
        }

        auto analysisResults = domain::PartitionAnalyzer::AnalyzeAll(
            allVolumes,
            context
        );

        for (const auto& result : analysisResults) {
            logger->Info(
                L"Volume " + result.volume.GetVolumePath() + 
                L" - Role: " + RoleToString(result.suggestedRole) +
                L" - Confidence: " + std::to_wstring(result.confidenceScore)
            );
        }

        co_return analysisResults;
    }

}
```

### Step 4 & 5: 설치 프로세스 (데이터 보존 모드)

```cpp
namespace application::workflow {

    Task<void> ExecuteInstallPreserveMode(
        const DiskAnalysisResult& diskResult,
        const SetupConfig& config
    ) {
        auto diskService = Container::Resolve<IDiskService>();
        auto volumeService = Container::Resolve<IVolumeService>();
        auto imagingService = Container::Resolve<IImagingService>();
        auto driverService = Container::Resolve<IDriverService>();
        auto logger = Container::Resolve<ILogger>();

        logger->Info(L"[STEP 4] Starting PRESERVE DATA mode installation");

        logger->Info(L"Step 4.1: Unmounting CDZ drive");
        co_await volumeService->UnmountDrive(L"CDZ");

        logger->Info(L"Step 4.2: Backing up user data");
        std::wstring tempPath = L"D:\\WinSetupTemp";
        
        for (const auto& backupPath : config.GetBackupPaths()) {
            std::wstring sourcePath = backupPath.Resolve(config.GetUserProfile());
            std::wstring targetPath = tempPath + L"\\" + backupPath.name;
            
            logger->Info(L"Backing up: " + sourcePath);
            
            co_await volumeService->MoveDirectory(
                sourcePath,
                targetPath,
                true
            );
        }

        logger->Info(L"Step 4.3: Formatting system partition");
        co_await diskService->FormatPartition(
            diskResult.selectedSystemDisk.GetIndex(),
            0,
            domain::FileSystemType::NTFS,
            true
        );

        logger->Info(L"Step 4.4: Applying WIM image");
        auto progressCallback = [&logger](const auto& progress) {
            logger->Info(
                L"Progress: " + 
                std::to_wstring(static_cast<int>(progress.percentage)) + 
                L"% - " + progress.currentFile
            );
        };

        co_await imagingService->ApplyImage(
            L"install.wim",
            1,
            L"C:\\",
            progressCallback
        );

        logger->Info(L"Step 4.5: Restoring user data");
        for (const auto& backupPath : config.GetBackupPaths()) {
            std::wstring sourcePath = tempPath + L"\\" + backupPath.name;
            std::wstring targetPath = backupPath.Resolve(config.GetUserProfile());
            
            logger->Info(L"Restoring: " + targetPath);
            
            co_await volumeService->MoveDirectory(
                sourcePath,
                targetPath,
                false
            );
        }

        logger->Info(L"Step 4.6: Injecting drivers");
        co_await driverService->InstallDrivers(L"C:\\", L"D:\\Drivers");

        logger->Info(L"Step 4.7: Cleaning up temporary files");
        co_await volumeService->DeleteDirectory(tempPath, true);

        logger->Info(L"[STEP 5] Starting reboot countdown");
        co_await UI::CountdownAndReboot(10);
    }

}
```

### Step 4 & 5: 설치 프로세스 (클린 모드)

```cpp
namespace application::workflow {

    Task<void> ExecuteInstallCleanMode(
        const DiskAnalysisResult& diskResult
    ) {
        auto diskService = Container::Resolve<IDiskService>();
        auto imagingService = Container::Resolve<IImagingService>();
        auto driverService = Container::Resolve<IDriverService>();
        auto logger = Container::Resolve<ILogger>();

        logger->Info(L"[STEP 4] Starting CLEAN INSTALL mode");

        logger->Info(L"Step 4.1: Cleaning disks");
        for (const auto& disk : diskResult.eligibleDisks) {
            logger->Info(L"Cleaning disk " + std::to_wstring(disk.GetIndex()));
            co_await diskService->CleanDisk(disk.GetIndex());
        }

        logger->Info(L"Step 4.2: Creating partition layout");
        
        abstractions::PartitionLayout layout;
        layout.style = abstractions::PartitionLayout::Style::GPT;
        
        layout.partitions = {
            domain::PartitionInfo{
                0,
                domain::PartitionType::EFI,
                550_MB,
                domain::FileSystemType::FAT32
            },
            domain::PartitionInfo{
                1,
                domain::PartitionType::MSR,
                128_MB,
                domain::FileSystemType::None
            },
            domain::PartitionInfo{
                2,
                domain::PartitionType::Primary,
                diskResult.selectedSystemDisk.GetSize() - 678_MB,
                domain::FileSystemType::NTFS
            }
        };

        co_await diskService->CreatePartitionLayout(
            diskResult.selectedSystemDisk.GetIndex(),
            layout
        );

        logger->Info(L"Step 4.3: Formatting partitions");
        co_await diskService->FormatPartition(
            diskResult.selectedSystemDisk.GetIndex(),
            0,
            domain::FileSystemType::FAT32,
            true
        );

        co_await diskService->FormatPartition(
            diskResult.selectedSystemDisk.GetIndex(),
            2,
            domain::FileSystemType::NTFS,
            true
        );

        logger->Info(L"Step 4.4: Applying WIM image");
        co_await imagingService->ApplyImage(
            L"install.wim",
            1,
            L"C:\\",
            nullptr
        );

        logger->Info(L"Step 4.5: Injecting drivers");
        co_await driverService->InstallDrivers(L"C:\\", L"D:\\Drivers");

        logger->Info(L"[STEP 5] Starting reboot countdown");
        co_await UI::CountdownAndReboot(10);
    }

}
```

### 전체 워크플로우 통합

```cpp
namespace application::workflow {

    Task<void> RunCompleteWorkflow(bool preserveDataMode) {
        auto logger = Container::Resolve<ILogger>();
        auto eventBus = Container::Resolve<IEventBus>();

        try {
            logger->Info(L"========================================");
            logger->Info(L"WinSetup Installation Started");
            logger->Info(L"========================================");

            auto systemResult = co_await InitializeSetup();
            if (!systemResult.HasValue()) {
                throw std::runtime_error("System analysis failed");
            }

            auto diskResult = co_await AnalyzeDisks(
                systemResult.Value().config.GetUserProfile()
            );
            if (!diskResult.HasValue()) {
                throw std::runtime_error("Disk analysis failed");
            }

            eventBus->Publish(std::make_shared<domain::DiskAnalyzedEvent>(
                diskResult.Value().allDisks,
                diskResult.Value().eligibleDisks,
                diskResult.Value().selectedSystemDisk,
                diskResult.Value().selectedDataDisk
            ));

            auto volumeResult = co_await AnalyzeVolumes(
                diskResult.Value(),
                systemResult.Value().config.GetUserProfile()
            );

            if (preserveDataMode) {
                co_await ExecuteInstallPreserveMode(
                    diskResult.Value(),
                    systemResult.Value().config
                );
            } else {
                co_await ExecuteInstallCleanMode(
                    diskResult.Value()
                );
            }

            logger->Info(L"========================================");
            logger->Info(L"WinSetup Installation Completed");
            logger->Info(L"========================================");

        } catch (const std::exception& ex) {
            logger->Fatal(L"Installation failed: " + 
                         std::wstring(ex.what(), ex.what() + strlen(ex.what())));
            throw;
        }
    }

}
```

---

## 구현 가이드

### Phase 1: Foundation & Domain (1-2주)

**목표:** 완전히 격리된 Domain 계층 구축

**작업:**
1. `domain/primitives/Expected.h`, `Result.h`, `Error.h`
2. `domain/valueobjects/` 모든 값 객체
3. `domain/entities/` 모든 엔티티
4. `domain/services/DiskSortingService`, `PartitionAnalyzer`
5. `domain/specifications/` 모든 Specification
6. `domain/events/` 모든 Domain Events

**검증:**
```cpp
// Domain 계층이 외부 의존성 없이 컴파일되는지 확인
#include <domain/entities/DiskInfo.h>
// Windows.h 없이 컴파일 성공해야 함

TEST(DomainTest, NoPlatformDependencies) {
    // Domain 객체 생성 및 사용
    domain::DiskInfo disk(0, domain::BusType::NVMe, 512_GB, L"path");
    EXPECT_EQ(disk.GetBusType(), domain::BusType::NVMe);
}
```

### Phase 2: Abstractions (1주)

**목표:** 모든 인터페이스 정의

**작업:**
1. `abstractions/repositories/` 인터페이스
2. `abstractions/services/` 인터페이스
3. `abstractions/infrastructure/` 인터페이스

**검증:**
- 모든 인터페이스가 Domain 타입만 사용
- 구현 코드 없음

### Phase 3: Win32 Adapters - Core (2주)

**목표:** 저수준 API 구현

**작업:**
1. `Win32TypeMapper` 구현 및 테스트
2. `IOCTLWrapper` 구현 (모든 IOCTL 래핑)
3. `Win32DiskService` 구현 (병렬 열거 포함)
4. `SMBIOSParser` 구현
5. `Win32SystemInfoService` 구현

**검증:**
```cpp
TEST(Win32DiskServiceTest, EnumeratesDisksInParallel) {
    Win32DiskService service(logger);
    auto result = service.EnumerateDisks();
    
    ASSERT_TRUE(result.HasValue());
    EXPECT_GT(result.Value().size(), 0);
    
    // 모든 disk가 Domain 타입 사용
    for (const auto& disk : result.Value()) {
        EXPECT_NE(disk.GetBusType(), domain::BusType::Unknown);
    }
}
```

### Phase 4: Application Layer (2주)

**목표:** Use Cases 및 이벤트 시스템

**작업:**
1. `EventBus` 구현
2. `TaskScheduler` 구현
3. 모든 Use Cases 구현
4. Event Handlers 구현

**검증:**
```cpp
TEST(AnalyzeDisksUseCaseTest, WorksWithMocks) {
    auto mockDiskService = std::make_shared<MockDiskService>();
    auto mockEventBus = std::make_shared<MockEventBus>();
    
    AnalyzeDisksUseCase useCase(mockDiskService, mockEventBus, logger);
    
    EXPECT_CALL(*mockDiskService, EnumerateDisks())
        .WillOnce(Return(CreateTestDisks()));
    
    bool eventPublished = false;
    EXPECT_CALL(*mockEventBus, Publish(_))
        .WillOnce([&](auto) { eventPublished = true; });
    
    auto result = useCase.Execute(L"testuser");
    
    EXPECT_TRUE(result.HasValue());
    EXPECT_TRUE(eventPublished);
}
```

### Phase 5: Imaging & Threading (2주)

**목표:** 고성능 이미징 및 멀티스레딩

**작업:**
1. `IOCompletionPort` 구현
2. `Win32ThreadPool` 구현
3. `WimlibAdapter` 구현
4. `DismAdapter` 구현

**검증:**
- 실제 WIM 파일로 추출 테스트
- IOCP 성능 측정

### Phase 6: Integration & Workflow (2주)

**목표:** 전체 워크플로우 통합

**작업:**
1. Main.cpp 작성
2. DI Container 구현
3. 전체 워크플로우 통합
4. End-to-End 테스트

**검증:**
- WinPE 환경에서 실제 설치 테스트
- 성능 측정 (디스크 분석 < 10초, 이미지 적용 시간)

---

## 성능 목표

### 디스크 분석
- **목표:** 10개 디스크 분석 < 5초
- **방법:** IOCP 기반 병렬 열거

### 이미지 적용
- **목표:** 10GB WIM → 평균 메인보드 예상 시간 준수
- **방법:** 
  - `WIMLIB_EXTRACT_FLAG_SEQUENTIAL`
  - 멀티코어 활용 (wimlib 내장)
  - 코어 제한 (시스템 리소스 80% 이하)

### 메모리 사용량
- **목표:** WinPE 환경에서 < 512MB
- **방법:**
  - RAII 기반 자동 해제
  - 대용량 버퍼 사전 할당
  - 스트리밍 처리

---

## 마무리

이 계획서는 **완벽한 클린 아키텍처**와 **최고 성능의 저수준 구현**을 결합한 마스터피스 구현 가이드입니다.

**핵심 달성 사항:**
✅ Domain 계층 완전 격리 (Windows.h 없음)
✅ IOCTL 기반 저수준 디스크 제어
✅ IOCP 기반 병렬 처리
✅ SMBIOS 직접 파싱
✅ wimlib 고성능 이미징
✅ Task Dependency Graph
✅ 완전한 테스트 가능성

이 구조를 따르면 **교과서적인 클린 아키텍처 + 실전 고성능 최적화**를 모두 달성할 수 있습니다.
