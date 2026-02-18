# WinSetup 클린 아키텍처 위반 전수 검토 보고서

## 🔴 Critical — 즉시 수정 필요

### 1. `MainViewModel`이 구체 UseCase 클래스를 직접 의존

**위치**: `src/application/viewmodels/MainViewModel.h`

```cpp
// ❌ 위반: Application 계층이 구체 클래스에 의존
#include <application/usecases/system/LoadConfigurationUseCase.h>
#include <application/usecases/system/AnalyzeSystemUseCase.h>

class MainViewModel : public abstractions::IMainViewModel {
    std::shared_ptr<LoadConfigurationUseCase> mLoadConfigUseCase;    // ❌
    std::shared_ptr<AnalyzeSystemUseCase>     mAnalyzeSystemUseCase; // ❌
};
```

**문제**: UseCase는 인터페이스 없이 구체 클래스만 존재하여 DIP(의존성 역전 원칙) 위반.  
`AnalyzeSystemUseCase`를 Mock으로 교체하거나 다른 구현으로 바꿀 수 없어 단위 테스트 불가.

**개선안**: 각 UseCase에 인터페이스를 추가하고 ViewModel은 인터페이스만 의존.

```cpp
// abstractions/usecases/ILoadConfigurationUseCase.h
namespace winsetup::abstractions {
    class ILoadConfigurationUseCase {
    public:
        virtual ~ILoadConfigurationUseCase() = default;
        [[nodiscard]] virtual domain::Expected<std::shared_ptr<domain::SetupConfig>>
            Execute(const std::wstring& configPath = L"config.ini") = 0;
    };
}

// abstractions/usecases/IAnalyzeSystemUseCase.h
namespace winsetup::abstractions {
    class IAnalyzeSystemUseCase {
    public:
        virtual ~IAnalyzeSystemUseCase() = default;
        [[nodiscard]] virtual domain::Expected<std::shared_ptr<domain::SystemInfo>>
            Execute() = 0;
    };
}
```

```cpp
// ✅ 수정 후 MainViewModel.h
#include <abstractions/usecases/ILoadConfigurationUseCase.h>
#include <abstractions/usecases/IAnalyzeSystemUseCase.h>

class MainViewModel : public abstractions::IMainViewModel {
    std::shared_ptr<abstractions::ILoadConfigurationUseCase> mLoadConfigUseCase;
    std::shared_ptr<abstractions::IAnalyzeSystemUseCase>     mAnalyzeSystemUseCase;
};
```

---

### 2. `IMainViewModel`이 Domain 엔티티를 직접 노출

**위치**: `src/abstractions/ui/IMainViewModel.h`

```cpp
// ❌ 위반: Abstractions 계층이 Domain Entity를 직접 반환
#include <domain/entities/SetupConfig.h>

class IMainViewModel : public IPropertyChanged {
    virtual std::vector<domain::InstallationType> GetInstallationTypes() const = 0; // ❌
};
```

**문제**: `abstractions/ui` 계층이 `domain/entities`를 직접 참조.  
View(Adapter 계층)가 Domain 타입을 알게 되어 계층 격리 원칙 위반.  
`InstallationType`이 변경되면 View까지 재컴파일 필요.

**개선안**: UI 전용 DTO 구조체를 `abstractions/ui`에 정의.

```cpp
// abstractions/ui/InstallationTypeDto.h
namespace winsetup::abstractions {
    struct InstallationTypeDto {
        std::wstring key;
        std::wstring description;
    };
}

// IMainViewModel.h — Domain include 제거 후
#include <abstractions/ui/InstallationTypeDto.h>

virtual std::vector<InstallationTypeDto> GetInstallationTypes() const = 0; // ✅
```

---

### 3. `IFileCopyService`의 네임스페이스 불일치

**위치**: `src/abstractions/services/storage/IFileCopyService.h`

```cpp
// ❌ 위반: 다른 인터페이스는 domain::Expected<T> 인데 이 파일만 완전 한정 네임스페이스 사용
[[nodiscard]] virtual winsetup::domain::Expected<void> CopyFile(...) = 0;
[[nodiscard]] virtual winsetup::domain::Expected<void> CopyDirectory(...) = 0;
```

**문제**: 나머지 인터페이스는 모두 `domain::Expected<T>` 형태인데 이 파일만 `winsetup::domain::Expected<T>` 사용.  
추후 네임스페이스 변경 시 이 파일만 누락될 위험.

**개선안**: `namespace winsetup::abstractions { }` 블록 안이므로 `domain::Expected<void>`로 통일.

```cpp
// ✅ 수정 후
[[nodiscard]] virtual domain::Expected<void> CopyFile(...) = 0;
[[nodiscard]] virtual domain::Expected<void> CopyDirectory(...) = 0;
```

---

## 🟠 Major — 가능한 빠른 수정 권장

### 4. `ServiceRegistration`이 `DIContainer::Register<>` 미활용

**위치**: `src/main/ServiceRegistration.cpp`

```cpp
// ❌ 구체 클래스를 직접 생성하여 RegisterInstance로 등록
container.RegisterInstance(std::make_shared<AnalyzeSystemUseCase>(
    container.Resolve<ISystemInfoService>().Value(),
    container.Resolve<ILogger>().Value()
));
```

**문제**: `RegisterInstance()`로 수동 생성하면 DI 컨테이너가 의존성 그래프를 관리하지 못함.  
`Register<TInterface, TImpl>()`을 사용해야 컨테이너가 생명주기와 의존성을 자동 관리.

**개선안**: `Register<>` 방식으로 통일.

```cpp
// ✅ 수정 후
container.Register<abstractions::ILoadConfigurationUseCase,
                   application::LoadConfigurationUseCase>(ServiceLifetime::Singleton);

container.Register<abstractions::IAnalyzeSystemUseCase,
                   application::AnalyzeSystemUseCase>(ServiceLifetime::Singleton);
```

---

### 5. `StatusPanel`이 ViewModel 상태를 로컬 멤버로 이중 저장

**위치**: `src/adapters/ui/win32/panels/StatusPanel.h/.cpp`

```cpp
// ❌ 위반: View가 ViewModel 상태를 로컬로 복사 (이중 상태 관리)
class StatusPanel : public abstractions::IWidget {
    std::wstring mStatusText;       // ❌ ViewModel에 이미 있는 값 복사
    std::wstring mTypeDescription;  // ❌ 동일
};

// PropertyChanged 수신 시마다 수동 동기화 필요
void StatusPanel::OnPropertyChanged(const std::wstring& propertyName) {
    if (propertyName == L"StatusText")
        mStatusText = mViewModel->GetStatusText(); // 중복 동기화
}
```

**문제**: PropertyChanged 누락 시 UI와 상태 불일치 발생.  
MVVM에서 View는 렌더링 시점에 ViewModel을 직접 조회해야 함.

**개선안**: 로컬 캐시 멤버 제거, `OnPaint` 시점에 ViewModel을 직접 조회.

```cpp
// ✅ 수정 후 — mStatusText, mTypeDescription 멤버 변수 삭제
void StatusPanel::DrawStatusText(HDC hdc) const {
    const auto text = mViewModel ? mViewModel->GetStatusText() : L"Ready";
    // ... 렌더링
}

void StatusPanel::DrawTypeDescription(HDC hdc) const {
    const auto text = mViewModel ? mViewModel->GetTypeDescription() : L"";
    // ... 렌더링
}

// OnPropertyChanged는 InvalidateRect만 수행
void StatusPanel::OnPropertyChanged(const std::wstring& propertyName) {
    if (!mhParent) return;
    if (propertyName == L"StatusText" || propertyName == L"TypeDescription")
        InvalidateRect(mhParent, nullptr, TRUE);
}
```

---

### 6. `ActionPanel`이 타이머 생명주기를 소유 — View가 프레젠테이션 로직 제어

**위치**: `src/adapters/ui/win32/panels/ActionPanel.cpp`

```cpp
// ❌ 위반: View가 타이밍 시작/종료 조건을 직접 판단
void ActionPanel::OnTimer(UINTPTR timerId) {
    mViewModel->TickTimer();
    if (mViewModel->GetProgress() >= 100)
        StopTimer(); // ❌ 종료 조건을 View가 판단
}

void ActionPanel::OnPropertyChanged(const std::wstring& propertyName) {
    if (propertyName == L"IsProcessing") {
        if (mViewModel->IsProcessing())
            StartTimer(); // ❌ View가 타이머 시작 결정
        else
            StopTimer();  // ❌ View가 타이머 종료 결정
    }
}
```

**문제**: 복수의 View가 붙거나 테스트 환경에서 `WM_TIMER`가 발생하지 않으면 `TickTimer()`가 호출되지 않음.  
타이머 생명주기는 프레젠테이션 로직이므로 ViewModel이 제어해야 함.

**개선안**: `IMainViewModel`에 타이머 제어 인터페이스 추가, `Win32MainWindow`에서 `WM_TIMER` 위임.

```cpp
// abstractions/ui/IMainViewModel.h 에 추가
virtual void OnTimerTick() = 0; // ✅ ViewModel이 내부에서 진행 여부 자체 판단

// ActionPanel — StartTimer/StopTimer 제거, 타이머는 Win32MainWindow가 단일 소유
// Win32MainWindow::OnTimer()
void Win32MainWindow::OnTimer(WPARAM timerId) {
    if (mViewModel)
        mViewModel->OnTimerTick(); // ✅ ViewModel에 위임만
}
```

---

### 7. `OptionPanel::Create`의 순서 의존적 초기화

**위치**: `src/adapters/ui/win32/panels/OptionPanel.cpp`

```cpp
// ❌ 위반: Create와 SetViewModel 호출 순서에 초기 UI 상태가 종속됨
void OptionPanel::Create(HWND hParent, ...) {
    mbtnDataPreserve.Create(...);
    mbtnBitlocker.Create(...);

    // mViewModel이 nullptr이면 초기값 적용 불가
    if (mViewModel) {
        mbtnDataPreserve.SetChecked(mViewModel->GetDataPreservation());
        mbtnBitlocker.SetChecked(mViewModel->GetBitlockerEnabled());
    }
}
```

**문제**: `Win32MainWindow::InitializeWidgets()`에서 `SetViewModel()` 후 `Create()` 순서를 지키고 있으나  
이 순서 의존성이 컴파일 타임에 강제되지 않아 깨지기 쉬운 구조.

**개선안**: `Create` 시그니처에서 ViewModel을 직접 파라미터로 수신, `SetViewModel()` 메서드 제거.

```cpp
// ✅ 수정 후 — 모든 Panel 공통 적용
void OptionPanel::Create(
    HWND hParent, HINSTANCE hInstance,
    int x, int y, int width, int height,
    std::shared_ptr<abstractions::IMainViewModel> viewModel  // ✅ 생성 시점에 강제
);
// SetViewModel() 별도 메서드 제거
```

---

## 🟡 Minor — 중장기 개선 권장

### 8. `DIContainer`의 `std::shared_ptr<void>` 타입 소거

**위치**: `src/application/core/DIContainer.h`

```cpp
// ⚠️ README 원칙 "void* 금지"의 경계선
std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;
```

**문제**: `shared_ptr<void>`는 deleter가 올바르게 캡처되어 실용적으로는 안전하나,  
README 설계 원칙 "void* 금지, 컴파일 타임 타입 체크"에 정면 위배.

**개선안**: `std::any` 또는 타입 안전 래퍼로 교체.

```cpp
// ✅ 개선안
std::unordered_map<std::type_index, std::any> mSingletons;

// Resolve 시
auto& anyVal = singletonIt->second;
return std::any_cast<std::shared_ptr<TInterface>>(anyVal);
```

---

### 9. `SetupConfig::ResolveBackupPath`의 드라이브 문자 하드코딩

**위치**: `src/domain/entities/SetupConfig.cpp`

```cpp
// ❌ 도메인 로직에 플랫폼 규칙 하드코딩
if (mHasDataPartition)
    resolved.replace(pos, 13, L"D:\\" + mUserProfile); // ❌ D: 하드코딩
else
    resolved.replace(pos, 13, L"C:\\" + mUserProfile); // ❌ C: 하드코딩
```

**문제**: 드라이브 문자가 바뀌면 Domain 코드를 수정해야 함.  
Domain은 외부 환경(디스크 레이아웃)을 몰라야 함.

**개선안**: `config.ini`에 드라이브 경로를 명시하거나 `SetupConfig`에 별도 필드로 추출.

```ini
; config.ini 에 추가
[PATHS]
SYSTEMDRIVE=C:
DATADRIVE=D:
```

```cpp
// SetupConfig.cpp — 하드코딩 제거
std::wstring SetupConfig::ResolveBackupPath(const std::wstring& path) const {
    const auto& drive = mHasDataPartition ? mDataDrive : mSystemDrive;
    resolved.replace(pos, 13, drive + L"\\" + mUserProfile); // ✅
}
```

---

### 10. `DomainEvent`와 `IEvent`의 이중 계층구조 미연결

**위치**: `src/domain/events/DomainEvent.h` vs `src/abstractions/infrastructure/messaging/IEvent.h`

```cpp
// domain/events/DomainEvent.h
class DomainEvent { ... }; // IEvent를 상속하지 않음 ❌

// abstractions/infrastructure/messaging/IEventBus.h
template<typename TEvent>
domain::Expected<void> Publish(TEvent event, ...) {
    static_assert(std::is_base_of_v<IEvent, TEvent>, // DomainEvent 파생 클래스 통과 불가 ❌
                  "TEvent must derive from IEvent");
    ...
}
```

**문제**: `DiskAnalyzedEvent` 등 모든 Domain 이벤트가 `IEventBus::Publish()`에 전달 불가.  
EventBus 인프라가 실질적으로 사용되지 못하는 상태.

**개선안**: `DomainEvent`를 `abstractions::EventBase<TDerived>` 기반으로 통합.

```cpp
// ✅ 수정 후 domain/events/DomainEvent.h
#include <abstractions/infrastructure/messaging/IEvent.h> // Domain이 Abstractions 참조는 허용

template<typename TDerived>
class DomainEvent : public abstractions::EventBase<TDerived> {
    // 도메인 공통 속성 추가 가능
};

// DiskAnalyzedEvent.h
class DiskAnalyzedEvent : public DomainEvent<DiskAnalyzedEvent> {
public:
    static std::wstring StaticEventType() { return L"DiskAnalyzed"; }
    std::wstring ToString() const override { ... }
    std::unique_ptr<abstractions::IEvent> Clone() const override { ... }
};
```

---

## 전체 요약

| # | 위치 | 위반 원칙 | 심각도 |
|---|---|---|---|
| 1 | `MainViewModel.h` | DIP — UseCase 구체 클래스 직접 의존 | 🔴 Critical |
| 2 | `IMainViewModel.h` | 계층 격리 — Abstraction이 Domain Entity 노출 | 🔴 Critical |
| 3 | `IFileCopyService.h` | 코드 일관성 — 네임스페이스 혼용 | 🔴 Critical |
| 4 | `ServiceRegistration.cpp` | DI 컨테이너 활용 미흡 | 🟠 Major |
| 5 | `StatusPanel.h/.cpp` | MVVM — View 로컬 상태 이중 관리 | 🟠 Major |
| 6 | `ActionPanel.cpp` | MVVM — 타이머 생명주기 View에 종속 | 🟠 Major |
| 7 | `OptionPanel.cpp` | 순서 의존적 초기화 | 🟠 Major |
| 8 | `DIContainer.h` | `shared_ptr<void>` 타입 소거 | 🟡 Minor |
| 9 | `SetupConfig.cpp` | 도메인 로직 하드코딩 | 🟡 Minor |
| 10 | `DomainEvent` vs `IEvent` | 이중 계층구조 미연결 | 🟡 Minor |

> **수정 우선순위**: 1번(UseCase 인터페이스 추출) → 2번(DTO 도입) → 10번(EventBus 연결) 순으로 진행 시 테스트 가능성이 가장 빠르게 향상됩니다.
