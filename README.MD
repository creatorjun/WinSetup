// README.md
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
7. **MVVM 패턴**: UI와 비즈니스 로직의 완전한 분리

### 기술 스택
- **언어**: C++20/23 (Concepts, Coroutines, Ranges)
- **플랫폼**: Windows PE (Windows 10/11)
- **빌드**: Visual Studio 2022 Solution (.sln + .vcxproj)
- **UI 패턴**: MVVM (Model-View-ViewModel)
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
- **Config.ini 로드**: < 100ms

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
│  ✓ ViewModels (MVVM)                    │
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
│  ✓ INI 파일 파서                         │
│  ✓ Win32 View (MVVM)                    │
└─────────────────────────────────────────┘
            ↑
            │ (모든 것 연결)
┌─────────────────────────────────────────┐
│  Main (Layer 4)                         │
│  ✓ Composition Root                     │
│  ✓ Service Registration                 │
└─────────────────────────────────────────┘
```

### 2. MVVM 패턴 적용

```
┌─────────────────────────────────────────┐
│  View (Win32MainWindow)                 │
│  - 순수 UI 렌더링                        │
│  - 사용자 입력 수신                      │
│  - ViewModel에게 명령 전달               │
│  - ViewModel 속성 변경 감지              │
└─────────────────────────────────────────┘
            ↓ IPropertyChanged
┌─────────────────────────────────────────┐
│  ViewModel (MainViewModel)              │
│  - 프레젠테이션 로직                     │
│  - 상태 관리 (StatusText, Progress)     │
│  - UseCase 호출                         │
│  - 이벤트 구독                           │
└─────────────────────────────────────────┘
            ↓ Domain/Application
┌─────────────────────────────────────────┐
│  Model (Entities, UseCases)             │
│  - 비즈니스 로직                         │
│  - Domain Entities                      │
│  - Services                             │
└─────────────────────────────────────────┘
```

**핵심 원칙**:
- View는 ViewModel의 인터페이스만 알고 있음
- ViewModel은 View를 전혀 모름 (IPropertyChanged 콜백만 사용)
- View는 HWND, HDC 등 Win32 타입만 다룸
- ViewModel은 std::wstring, bool, int 등 표준 타입만 다룸

---

## 📂 폴더 구조

```
WinSetup/
│
├── WinSetup.sln
├── README.md
├── ARCHITECTURE.md
│
├── WinSetup/
│   ├── WinSetup.vcxproj
│   ├── WinSetup.vcxproj.filters
│   │
│   ├── src/
│   │   │
│   │   ├── abstractions/
│   │   │   │
│   │   │   ├── ui/
│   │   │   │   ├── IWindow.h
│   │   │   │   ├── IWidget.h
│   │   │   │   ├── IProgressBar.h
│   │   │   │   ├── IMainViewModel.h          # ✅ NEW: MVVM ViewModel 인터페이스
│   │   │   │   └── IPropertyChanged.h        # ✅ NEW: PropertyChanged 인터페이스
│   │   │   │
│   │   │   └── repositories/
│   │   │       └── IConfigRepository.h        # ✅ Config 저장소 인터페이스
│   │   │
│   │   ├── application/
│   │   │   │
│   │   │   ├── viewmodels/                    # ✅ NEW: MVVM ViewModels
│   │   │   │   ├── MainViewModel.h
│   │   │   │   └── MainViewModel.cpp
│   │   │   │
│   │   │   └── usecases/
│   │   │       └── system/
│   │   │           ├── LoadConfigurationUseCase.h  # ✅ NEW: Config 로드 UseCase
│   │   │           └── LoadConfigurationUseCase.cpp
│   │   │
│   │   ├── domain/
│   │   │   │
│   │   │   └── entities/
│   │   │       ├── SetupConfig.h              # ✅ Config 엔티티
│   │   │       └── SetupConfig.cpp
│   │   │
│   │   ├── adapters/
│   │   │   │
│   │   │   ├── persistence/                   # ✅ NEW: 영구 저장 계층
│   │   │   │   │
│   │   │   │   ├── config/
│   │   │   │   │   ├── IniParser.h           # ✅ INI 파일 파서
│   │   │   │   │   ├── IniParser.cpp
│   │   │   │   │   ├── IniConfigRepository.h # ✅ Config Repository 구현
│   │   │   │   │   └── IniConfigRepository.cpp
│   │   │   │   │
│   │   │   │   └── filesystem/
│   │   │   │       ├── Win32FileSystem.h
│   │   │   │       └── Win32FileSystem.cpp
│   │   │   │
│   │   │   ├── platform/win32/
│   │   │   │   └── logging/
│   │   │   │       ├── Win32Logger.h         # ✅ log/log.txt 로깅
│   │   │   │       └── Win32Logger.cpp
│   │   │   │
│   │   │   └── ui/win32/                     # ✅ NEW: MVVM View 구현
│   │   │       ├── Win32MainWindow.h
│   │   │       ├── Win32MainWindow.cpp
│   │   │       ├── Win32ProgressBar.h
│   │   │       └── Win32ProgressBar.cpp
│   │   │
│   │   └── main/
│   │       ├── Main.cpp
│   │       ├── ServiceRegistration.h          # ✅ DI 등록
│   │       └── ServiceRegistration.cpp
│   │
│   └── config.ini                             # ✅ NEW: 설정 파일
```

---

## 🆕 추가된 구성 요소 상세 설명

### 1. MVVM 패턴 구현

#### abstractions/ui/IPropertyChanged.h
```cpp
#pragma once

#include <functional>
#include <string>

namespace winsetup::abstractions {

    using PropertyChangedCallback = std::function<void(const std::wstring& propertyName)>;

    class IPropertyChanged {
    public:
        virtual ~IPropertyChanged() = default;
        
        virtual void AddPropertyChangedHandler(PropertyChangedCallback callback) = 0;
        virtual void RemoveAllPropertyChangedHandlers() = 0;

    protected:
        virtual void NotifyPropertyChanged(const std::wstring& propertyName) = 0;
    };

}
```

#### abstractions/ui/IMainViewModel.h
```cpp
#pragma once

#include <string>
#include "IPropertyChanged.h"
#include <domain/primitives/Expected.h>

namespace winsetup::abstractions {

    class IMainViewModel : public IPropertyChanged {
    public:
        virtual ~IMainViewModel() = default;

        [[nodiscard]] virtual std::wstring GetStatusText() const = 0;
        virtual void SetStatusText(const std::wstring& text) = 0;

        [[nodiscard]] virtual std::wstring GetWindowTitle() const = 0;
        virtual void SetWindowTitle(const std::wstring& title) = 0;

        [[nodiscard]] virtual bool IsInitializing() const = 0;
        [[nodiscard]] virtual bool IsProcessing() const = 0;
        [[nodiscard]] virtual bool IsCompleted() const = 0;

        virtual domain::Expected<void> Initialize() = 0;
    };

}
```

#### application/viewmodels/MainViewModel.h
```cpp
#pragma once

#include <abstractions/ui/IMainViewModel.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <domain/entities/SetupConfig.h>
#include <memory>
#include <vector>
#include <string>

namespace winsetup::application {

    class MainViewModel : public abstractions::IMainViewModel {
    public:
        explicit MainViewModel(std::shared_ptr<abstractions::ILogger> logger);
        ~MainViewModel() override = default;

        [[nodiscard]] std::wstring GetStatusText() const override;
        void SetStatusText(const std::wstring& text) override;

        [[nodiscard]] std::wstring GetWindowTitle() const override;
        void SetWindowTitle(const std::wstring& title) override;

        [[nodiscard]] bool IsInitializing() const override { return mIsInitializing; }
        [[nodiscard]] bool IsProcessing() const override { return mIsProcessing; }
        [[nodiscard]] bool IsCompleted() const override { return mIsCompleted; }

        domain::Expected<void> Initialize() override;

        void AddPropertyChangedHandler(abstractions::PropertyChangedCallback callback) override;
        void RemoveAllPropertyChangedHandlers() override;

        [[nodiscard]] std::shared_ptr<domain::SetupConfig> GetConfig() const { return mConfig; }

    protected:
        void NotifyPropertyChanged(const std::wstring& propertyName) override;

    private:
        domain::Expected<void> LoadConfiguration();

        std::shared_ptr<abstractions::ILogger> mLogger;
        std::shared_ptr<domain::SetupConfig> mConfig;
        
        std::wstring mStatusText;
        std::wstring mWindowTitle;
        
        bool mIsInitializing;
        bool mIsProcessing;
        bool mIsCompleted;

        std::vector<abstractions::PropertyChangedCallback> mPropertyChangedHandlers;
    };

}
```

### 2. Config.ini 파싱 시스템

#### Config.ini 구조
```ini
[USERPROFILE]
USERPROFILE=kdic

[PARTITION]
DATAPARTITION=TRUE

[BACKUP]
DESKTOP={USERPROFILE}/desktop
DOWNLOADS={USERPROFILE}/Downloads
DOCUMENTS={USERPROFILE}/Documents
PICTURES={USERPROFILE}/Pictures
MUSIC={USERPROFILE}/Music
VIDEOS={USERPROFILE}/Videos

NPKI={USERPROFILE}/AppData/Local/NPKI
STICKY={USERPROFILE}/AppData/Local/Packages/Microsoft.MicrosoftStickyNotes_8wekyb3d8bbwe/LocalState

[TYPES]
내부망=내부망 : 26-01-01 / 윈도우 11 24H1
외부망=외부망 : 26-01-01 / 윈도우 11 24H2
출장용=출장용 : 26-01-01 / 윈도우 11 24H3
자회사=자회사 : 26-01-01 / 윈도우 11 24H4

[TIMES]
15Z90S=120
```

#### domain/entities/SetupConfig.h
```cpp
#pragma once

#include <string>
#include <vector>
#include <map>

namespace winsetup::domain {

    struct BackupTarget {
        std::wstring name;
        std::wstring path;

        BackupTarget() = default;
        BackupTarget(const std::wstring& n, const std::wstring& p)
            : name(n), path(p) {}
    };

    struct InstallationType {
        std::wstring name;
        std::wstring description;

        InstallationType() = default;
        InstallationType(const std::wstring& n, const std::wstring& d)
            : name(n), description(d) {}
    };

    class SetupConfig {
    public:
        SetupConfig();
        ~SetupConfig() = default;

        [[nodiscard]] const std::wstring& GetUserProfile() const noexcept;
        void SetUserProfile(const std::wstring& profile);

        [[nodiscard]] bool HasDataPartition() const noexcept;
        void SetDataPartition(bool hasPartition) noexcept;

        [[nodiscard]] const std::vector<BackupTarget>& GetBackupTargets() const noexcept;
        void AddBackupTarget(const std::wstring& name, const std::wstring& path);

        [[nodiscard]] const std::vector<InstallationType>& GetInstallationTypes() const noexcept;
        void AddInstallationType(const std::wstring& name, const std::wstring& description);

        [[nodiscard]] bool HasEstimatedTime(const std::wstring& motherboardModel) const;
        [[nodiscard]] uint32_t GetEstimatedTime(const std::wstring& motherboardModel) const;
        void SetEstimatedTime(const std::wstring& motherboardModel, uint32_t seconds);

        [[nodiscard]] std::wstring ResolveBackupPath(const std::wstring& path) const;
        [[nodiscard]] bool IsValid() const noexcept;

    private:
        std::wstring mUserProfile;
        bool mHasDataPartition;
        std::vector<BackupTarget> mBackupTargets;
        std::vector<InstallationType> mInstallationTypes;
        std::map<std::wstring, uint32_t> mEstimatedTimes;
    };

}
```

#### adapters/persistence/config/IniParser.h
```cpp
#pragma once

#include <domain/primitives/Expected.h>
#include <string>
#include <map>
#include <vector>

namespace winsetup::adapters::persistence {

    class IniParser {
    public:
        using Section = std::map<std::wstring, std::wstring>;
        using IniData = std::map<std::wstring, Section>;

        IniParser() = default;
        ~IniParser() = default;

        [[nodiscard]] domain::Expected<IniData> Parse(const std::wstring& filePath);
        [[nodiscard]] domain::Expected<IniData> ParseContent(const std::wstring& content);

        [[nodiscard]] static std::wstring Trim(const std::wstring& str);
        [[nodiscard]] static std::wstring ToUpper(const std::wstring& str);

    private:
        [[nodiscard]] bool IsComment(const std::wstring& line) const;
        [[nodiscard]] bool IsSection(const std::wstring& line) const;
        [[nodiscard]] std::wstring ExtractSectionName(const std::wstring& line) const;
        [[nodiscard]] bool ParseKeyValue(const std::wstring& line, 
                                        std::wstring& key, 
                                        std::wstring& value) const;
    };

}
```

#### application/usecases/system/LoadConfigurationUseCase.h
```cpp
#pragma once

#include <domain/primitives/Expected.h>
#include <domain/entities/SetupConfig.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <memory>
#include <string>

namespace winsetup::application {

    class LoadConfigurationUseCase {
    public:
        explicit LoadConfigurationUseCase(
            std::shared_ptr<abstractions::ILogger> logger
        );
        ~LoadConfigurationUseCase() = default;

        [[nodiscard]] domain::Expected<std::shared_ptr<domain::SetupConfig>>
            Execute(const std::wstring& configPath = L"config.ini");

    private:
        std::shared_ptr<abstractions::ILogger> mLogger;
    };

}
```

### 3. Win32MainWindow MVVM 구현

#### adapters/ui/win32/Win32MainWindow.cpp (핵심 부분)
```cpp
namespace winsetup::adapters::ui {

    Win32MainWindow::Win32MainWindow(
        std::shared_ptr<abstractions::IMainViewModel> viewModel,
        std::shared_ptr<abstractions::ILogger> logger
    )
        : mViewModel(std::move(viewModel))
        , mLogger(std::move(logger))
        , mHwnd(nullptr)
    {
        // ViewModel의 속성 변경 구독
        if (mViewModel) {
            mViewModel->AddPropertyChangedHandler(
                [this](const std::wstring& propertyName) {
                    OnViewModelPropertyChanged(propertyName);
                }
            );
        }
    }

    void Win32MainWindow::OnViewModelPropertyChanged(const std::wstring& propertyName) {
        if (!mHwnd) return;

        if (propertyName == L"StatusText") {
            // UI 업데이트
            InvalidateRect(mHwnd, nullptr, TRUE);
        }
        else if (propertyName == L"WindowTitle") {
            SetWindowTextW(mHwnd, mViewModel->GetWindowTitle().c_str());
        }
    }

}
```

---

## 📅 업데이트된 구현 단계

### ✅ 완료된 Phase

#### Phase 1: 기초 인프라 (완료)
- ✅ domain/primitives/Error
- ✅ domain/primitives/Expected
- ✅ abstractions/infrastructure/logging/ILogger
- ✅ adapters/platform/win32/logging/Win32Logger (log/log.txt 경로)

#### Phase 2: Domain Entities (완료)
- ✅ domain/valueobjects (BusType, DiskType, FileSystemType 등)
- ✅ domain/entities/SetupConfig (Config.ini 파싱용)
- ✅ domain/primitives/Error (Validation, NotImplemented 추가)

#### Phase 3: Configuration System (완료)
- ✅ adapters/persistence/config/IniParser
- ✅ adapters/persistence/config/IniConfigRepository
- ✅ application/usecases/system/LoadConfigurationUseCase
- ✅ Config.ini 로드 및 파싱 완료

#### Phase 4: MVVM Pattern (완료)
- ✅ abstractions/ui/IPropertyChanged
- ✅ abstractions/ui/IMainViewModel
- ✅ application/viewmodels/MainViewModel
- ✅ adapters/ui/win32/Win32MainWindow (MVVM 적용)
- ✅ ViewModel 초기화 시 Config 자동 로드

#### Phase 5: DI Container (완료)
- ✅ application/core/DIContainer
- ✅ main/ServiceRegistration
- ✅ 모든 서비스 자동 등록 및 와이어링

### 🚧 진행 중인 Phase

#### Phase 6: Win32 Core Infrastructure (진행 중)
**현재 상태**: UniqueHandle, Win32Logger 완료
**남은 작업**:
- [ ] Win32TypeMapper
- [ ] Win32ErrorHandler
- [ ] Win32StringHelper

### 📋 미완료 Phase

#### Phase 7: Storage Adapters
- [ ] IOCTLWrapper + AsyncIOCTL
- [ ] Win32DiskService
- [ ] Win32VolumeService
- [ ] MFTScanner
- [ ] DiskTransaction

#### Phase 8: SMBIOS & System Info
- [ ] SMBIOSParser
- [ ] Win32SystemInfoService

#### Phase 9: Imaging Services
- [ ] WimlibAdapter
- [ ] DismAdapter

#### Phase 10: Use Cases
- [ ] InstallWindowsUseCase
- [ ] BackupUserDataUseCase
- [ ] 기타 UseCases

---

## 🎯 현재 작동 확인된 기능

### ✅ 정상 작동
1. **로깅 시스템**
   ```
   [INIT] Win32Logger initialized
   2026-02-18 03:52:43.202 [DEBUG] Window WM_CREATE received
   2026-02-18 03:52:43.222 [INFO ] Main window created successfully
   ```

2. **Config.ini 로드**
   ```
   2026-02-18 03:52:43.222 [INFO ] Loading configuration from: config.ini
   2026-02-18 03:52:43.235 [INFO ] Configuration loaded successfully
   2026-02-18 03:52:43.235 [INFO ]   User Profile: kdic
   2026-02-18 03:52:43.235 [INFO ]   Data Partition: Yes
   2026-02-18 03:52:43.235 [INFO ]   Backup Targets: 2
   2026-02-18 03:52:43.235 [INFO ]   Installation Types: 4
   ```

3. **MVVM 패턴**
   - MainViewModel 초기화 성공
   - PropertyChanged 이벤트 전파
   - View-ViewModel 바인딩 작동

4. **DI Container**
   - 서비스 자동 등록
   - 의존성 자동 주입
   - Singleton 라이프타임 관리

---

## 📊 진행률

### 전체 아키텍처
- Domain 계층: **60%** (기본 타입 완료, 엔티티 일부 완료)
- Abstractions 계층: **40%** (주요 인터페이스 정의됨)
- Application 계층: **50%** (MVVM + DI + Config 완료)
- Adapters 계층: **30%** (로깅, Config 완료, 스토리지 미완)
- Main 계층: **80%** (ServiceRegistration 완료)

### 기능별
- ✅ 로깅: 100%
- ✅ Config 로드: 100%
- ✅ MVVM: 100%
- ✅ DI: 100%
- ⏳ 디스크 서비스: 0%
- ⏳ 이미징: 0%
- ⏳ UI 컨트롤: 20%

---

## 🔄 변경 이력

### v1.1 (2026-02-18)
- ✅ MVVM 패턴 추가
- ✅ Config.ini 파싱 시스템 추가
- ✅ Win32Logger 경로 변경 (WinSetup.log → log/log.txt)
- ✅ ErrorCategory에 Validation, NotImplemented 추가
- ✅ IPropertyChanged 인터페이스 추가
- ✅ MainViewModel 구현 추가
- ✅ LoadConfigurationUseCase 추가
- ✅ 완료된 Phase 업데이트

### v1.0 (2026-02-15)
- 초기 계획서 작성

---

**계획서 업데이트일**: 2026년 2월 18일  
**버전**: 1.0
**작성자**: 예금보험공사 / IT운영부 / 한준희 과장  
**상태**: 진행 중 (Phase 6)
