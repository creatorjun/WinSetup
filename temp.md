# WinSetup 클린 아키텍처 위반 전수 검토 보고서

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


# WinSetup 잔여 개선 과제 (구현 완료 범위 기준)

현재 **87점 → 100점** 달성을 위한 3가지 잔여 과제입니다.

---

## 1. `IWidget` Win32 타입 → 플랫폼 독립 타입 추상화 (+7점)

**위치**: `src/abstractions/ui/IWidget.h`

### 문제
```cpp
// ❌ abstractions 계층에 Win32 플랫폼 타입 직접 노출
#include <Windows.h>

class IWidget {
    virtual void Create(HWND hParent, HINSTANCE hInstance,
                        int x, int y, int width, int height) = 0;
};
```
`abstractions` 계층 인터페이스가 `HWND`, `HINSTANCE` Win32 타입에 직접 의존.  
`IWidget`을 포함하는 모든 상위 헤더가 Win32 환경 없이 컴파일 불가.  
Mock 구현 시에도 Win32 SDK가 필수가 되어 단위 테스트 환경이 오염됨.

### 개선안

```cpp
// abstractions/ui/WidgetCreateParams.h — 신규 생성
// Windows.h include 없음
#pragma once
#include <cstdint>

namespace winsetup::abstractions {

using NativeWindowHandle   = void*;
using NativeInstanceHandle = void*;

struct WidgetCreateParams {
    NativeWindowHandle   parentHandle = nullptr;
    NativeInstanceHandle instanceHandle = nullptr;
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
};

} // namespace winsetup::abstractions
```

```cpp
// abstractions/ui/IWidget.h — Windows.h 제거
#pragma once
#include <abstractions/ui/WidgetCreateParams.h>
#include <string>

namespace winsetup::abstractions {

class IWidget {
public:
    virtual ~IWidget() = default;
    virtual void Create(const WidgetCreateParams& params) = 0;  // ✅ Win32 타입 없음
    virtual void OnPaint(void* hdc) = 0;
    virtual bool OnCommand(uintptr_t wParam, intptr_t lParam) = 0;
    virtual void OnTimer(uintptr_t timerId) = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual void OnPropertyChanged(const std::wstring& propertyName) = 0;
    [[nodiscard]] virtual bool IsValid() const noexcept = 0;
};

} // namespace winsetup::abstractions
```

```cpp
// adapters/ui/win32/panels/OptionPanel.cpp — Adapter 계층에서만 Win32 타입 복원
void OptionPanel::Create(const abstractions::WidgetCreateParams& params) {
    HWND     hParent   = static_cast<HWND>(params.parentHandle);    // ✅ Adapter에서만 캐스팅
    HINSTANCE hInstance = static_cast<HINSTANCE>(params.instanceHandle);
    // ...
}
```

---

## 2. `ServiceLocator` 제거 및 생성자 주입 완전 통일 (+4점)

**위치**: `src/application/core/ServiceLocator.h/.cpp`

### 문제
```cpp
// ❌ DIContainer와 ServiceLocator 두 가지 접근 방식 병존
// ServiceLocator — 전역 상태, 어디서든 호출 가능
ServiceLocator::Get<ILogger>()->Info(L"...");  // ❌ 의존성 흐름 추적 불가
ServiceLocator::Get<IDiskService>()->EnumerateDisks(); // ❌ 테스트 격리 불가
```

`ServiceLocator`는 의존성을 숨기는 Anti-pattern.  
어느 클래스에서 무엇을 사용하는지 생성자만 보고 알 수 없어 의존성 그래프 추적이 불가능함.  
테스트 시 Mock 교체를 위해 전역 상태를 조작해야 함.

### 개선안

```cpp
// ❌ 제거 대상
// src/application/core/ServiceLocator.h  → 삭제
// src/application/core/ServiceLocator.cpp → 삭제
```

```cpp
// ✅ 생성자 주입으로 완전 통일
// ServiceLocator::Get<ILogger>() 호출부를 전부 생성자 파라미터로 교체

// 기존 ServiceLocator 사용 예
class Win32DiskService {
    void SomeMethod() {
        auto logger = ServiceLocator::Get<ILogger>(); // ❌
        logger->Info(L"...");
    }
};

// ✅ 수정 후 — 생성자에서 주입
class Win32DiskService : public abstractions::IDiskService {
public:
    explicit Win32DiskService(std::shared_ptr<abstractions::ILogger> logger)
        : mLogger(std::move(logger)) {}  // ✅ 의존성 명시적 선언
private:
    std::shared_ptr<abstractions::ILogger> mLogger;
};
```

```cpp
// ServiceRegistration.cpp — ServiceLocator 초기화 코드 제거
// ❌ 제거
ServiceLocator::Initialize(container);

// ✅ 모든 의존성은 Register/Resolve 체인으로만 연결
```

---

## 3. `MainViewModel::Initialize()` 단계 분리 (+2점)

**위치**: `src/application/viewmodels/MainViewModel.cpp`

### 문제
```cpp
// ❌ 단일 메서드가 4가지 책임을 순차 처리
domain::Expected<void> MainViewModel::Initialize() {
    mIsInitializing = true;
    SetStatusText(L"Initializing...");      // 1. UI 상태 변경

    auto sysResult = RunAnalyzeSystem();    // 2. 시스템 분석 (외부 I/O)
    auto cfgResult = RunLoadConfiguration(); // 3. 설정 로드 (파일 I/O)

    mIsInitializing = false;
    SetStatusText(L"Ready");               // 4. UI 상태 복원
    NotifyPropertyChanged(L"InstallationTypes");
    return {};
}
```

SRP 위반 — UI 상태 관리, 시스템 분석, 설정 로드, 완료 처리가 단일 메서드에 집중됨.  
단계 중 하나가 실패했을 때 복구 로직이 복잡해지고, 각 단계를 독립적으로 테스트 불가.

### 개선안

```cpp
// ✅ 각 단계를 독립 메서드로 분리하고 IMainViewModel에 단계별 진입점 추가

// abstractions/ui/IMainViewModel.h 에 추가
virtual domain::Expected<void> Initialize()       = 0;  // 전체 오케스트레이션
virtual domain::Expected<void> AnalyzeSystem()    = 0;  // 시스템 분석만
virtual domain::Expected<void> LoadConfiguration() = 0; // 설정 로드만
```

```cpp
// MainViewModel.cpp — 단계 분리 후
domain::Expected<void> MainViewModel::Initialize() {
    SetInitializingState(true);

    if (auto r = AnalyzeSystem();    !r.HasValue()) { SetInitializingState(false); return r; }
    if (auto r = LoadConfiguration(); !r.HasValue()) { SetInitializingState(false); return r; }

    SetInitializingState(false);
    return {};
}

domain::Expected<void> MainViewModel::AnalyzeSystem() {
    SetStatusText(L"Reading system information...");
    auto result = mAnalyzeSystemUseCase->Execute();
    if (!result.HasValue()) return result.GetError();
    mSystemInfo = result.Value();
    return {};
}

domain::Expected<void> MainViewModel::LoadConfiguration() {
    SetStatusText(L"Loading configuration...");
    auto result = mLoadConfigUseCase->Execute(L"config.ini");
    if (!result.HasValue()) return result.GetError();
    mConfig = result.Value();
    ApplyConfigToState();   // 타이머 시간 등 상태 반영
    NotifyPropertyChanged(L"InstallationTypes");
    return {};
}

// private — UI 상태만 담당하는 단일 책임 메서드
void MainViewModel::SetInitializingState(bool initializing) {
    mIsInitializing = initializing;
    SetStatusText(initializing ? L"Initializing..." : L"Ready");
}

void MainViewModel::ApplyConfigToState() {
    if (!mConfig) return;
    const std::wstring model = mSystemInfo ? mSystemInfo->GetMotherboardModel() : L"";
    const uint32_t secs      = mConfig->GetEstimatedTime(model);
    mTotalSeconds     = secs > 0u ? secs : kDefaultTotalSeconds;
    mRemainingSeconds = mTotalSeconds;
    mElapsedSeconds   = 0u;
    mProgress         = 0;
}
```

---

## 개선 후 최종 점수 예상

| 과제 | 향상 | 누적 점수 |
|---|:---:|:---:|
| 현재 (10개 항목 적용 후) | — | **87점** |
| `IWidget` Win32 타입 추상화 | +7점 | 94점 |
| `ServiceLocator` 제거 | +4점 | 98점 |
| `MainViewModel::Initialize()` 분리 | +2점 | **≈ 100점** |
