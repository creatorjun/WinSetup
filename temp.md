코드를 충분히 파악했습니다. 이제 종합 검토 결과를 정리합니다.

***

# 클린 아키텍처 위반사항 검토 보고서

전반적으로 계층 분리, 인터페이스 기반 설계, DI 패턴이 매우 잘 구현되어 있습니다. 그러나 "교과서적 마스터피스"를 목표로 한다면 아래의 위반 사항들을 반드시 수정해야 합니다.

***

## 🔴 심각 (Dependency Rule 직접 위반)

### 1. `Win32VolumeService.h` — Adapters 헤더에 `Windows.h` 노출

```cpp
// adapters/platform/win32/storage/Win32VolumeService.h
#pragma once
#include <abstractions/services/storage/IVolumeService.h>
#include <abstractions/infrastructure/logging/ILogger.h>
#include <adapters/platform/win32/memory/UniqueHandle.h>
#include <memory>
#include <Windows.h>   // ← 헤더에 노출됨
```

`Windows.h`는 `.cpp`에서만 include해야 합니다. 이 헤더를 include하는 모든 파일로 플랫폼 오염이 전파됩니다.

**수정**: `Windows.h`를 `.cpp`로 이동. 헤더에서 `HANDLE`/`HWND` 타입이 필요 없도록 pimpl 패턴 또는 forward declaration 사용.

***

### 2. `TypeSelectorGroup.h` — Adapters UI가 Domain Entity를 직접 include

```cpp
// adapters/ui/win32/controls/TypeSelectorGroup.h
#include <domain/entities/SetupConfig.h>   // ← Domain 엔티티 직접 참조
```

Adapter 계층의 UI 컨트롤이 Domain 엔티티를 직접 참조하면 안 됩니다. UI는 반드시 Abstractions 인터페이스 또는 Application의 DTO만 참조해야 합니다.

**수정**: `domain::InstallationType` VO를 DTO나 abstractions에 노출하거나, `std::vector<std::wstring>` 같은 표준 타입으로 인터페이스를 정의.

***

### 3. `StatusPanel.h` — Adapters UI 헤더에 `Windows.h` + Win32 타입 직접 노출

```cpp
// adapters/ui/win32/panels/StatusPanel.h
#include <Windows.h>               // ← 헤더에 노출
...
void OnPaint(HDC hdc) override;     // HDC: Win32 타입이 추상 인터페이스에 노출
void Create(HWND hParent, ...);     // HWND: Win32 타입
```

`IWidget` 추상 인터페이스의 메서드 시그니처에 `HDC`, `HWND` 같은 Win32 타입이 올라가 있다면, Abstractions 계층이 플랫폼에 오염됩니다.

**수정**: `IWidget::OnPaint()`의 파라미터를 추상화하거나(`IPaintContext`), `OnPaint`를 Adapter 내부 구현 전용으로 격리.

***

## 🟠 경고 (설계 원칙 위반)

### 4. `AnalysisRepository` 위치 오류 — Adapters → Persistence에 Repository 구현이 있으나 추상화가 불완전

현재 구현된 파일 구조를 보면:
```
adapters/persistence/analysis/AnalysisRepository.h
abstractions/repositories/IAnalysisRepository.h
```

`AnalysisRepository`는 실제로는 **인메모리 캐시**(파일/DB가 없는 순수 메모리 저장소)임에도 `persistence/` 폴더에 배치되어 있습니다. 이는 의미론적 오류입니다.

**수정**: `adapters/persistence/analysis/` → `application/repositories/` 또는 `adapters/cache/` 로 이동.

***

### 5. `DIContainer` — `shared_ptr<void>` 저장 구조의 타입 안전 우회

```cpp
// application/core/DIContainer.h
std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;
// ...
return std::static_pointer_cast<TInterface>(instance);
```

README에서 *"소멸자 안전"*을 이유로 의도적으로 선택했다고 명시되어 있으나, `static_pointer_cast`를 통한 형 변환은 런타임 안전성이 보장되지 않습니다. 등록 타입(`TImplementation`)과 조회 타입(`TInterface`)이 상속 관계가 아닌 경우 UB가 발생합니다.

**수정**: `std::dynamic_pointer_cast` + 실패 시 `Expected` 에러 반환으로 안전성 보장. 또는 type-safe 팩토리 패턴 적용.

***

### 6. `Win32FileCopyService` — `WorkerContext`에 raw pointer 전달

```cpp
// Win32FileCopyService.h
struct WorkerContext {
    Win32FileCopyService* service = nullptr;   // ← raw pointer
    std::vector<CopyTask>* tasks = nullptr;    // ← raw pointer
    std::atomic<uint32_t>* taskIndex = nullptr;// ← raw pointer
    ...
};
```

`CreateThread`의 `LPVOID` 인자로 넘기기 위해 raw pointer를 사용하고 있습니다. 스레드보다 `WorkerContext` 소유자(`CopyDirectory`)가 먼저 종료되면 댕글링 포인터가 됩니다. 스택 변수의 주소를 스레드 함수에 전달하는 패턴은 수명 관리 버그의 전형적인 원인입니다.

**수정**: `WorkerContext`를 `shared_ptr<WorkerContext>`로 힙 할당 후 스레드에 전달. `WaitForMultipleObjects` 완료 후 해제 보장.

***

### 7. `AsyncIOCTL` — 매 비동기 작업마다 `CreateThread` 호출

```cpp
// AsyncIOCTL.cpp
HANDLE hThread = CreateThread(nullptr, 0,
    [](LPVOID lpParam) -> DWORD {
        // ...
        delete context;
        return 0;
    },
    new std::pair<...>(this, op), 0, nullptr);
if (hThread)
    CloseHandle(hThread);  // ← 핸들을 즉시 닫음
```

`CreateThread`를 즉시 `CloseHandle`하면 스레드 핸들을 추적할 수 없습니다. 동시에 `IThreadPool` 추상화가 이미 정의되어 있음에도 `AsyncIOCTL` 내부에서 직접 스레드를 생성하는 것은 **의존성 역전 원칙 위반**입니다.

**수정**: `IThreadPool` / `IExecutor` 인터페이스를 생성자에서 주입받아 사용.

***

### 8. `Win32MainWindow` — View에서 `IMainViewModel::Initialize()` 직접 호출 의존

```cpp
// MainViewModel.cpp
domain::Expected<void> MainViewModel::Initialize() {
    // RunAnalyzeSystem, RunLoadConfiguration 직접 호출
    auto sysResult = RunAnalyzeSystem();
    auto cfgResult = RunLoadConfiguration();
    ...
}
```

`ViewModel::Initialize()`가 UseCase들을 동기적으로 순차 실행합니다. 이는 UI 스레드를 블로킹하며, `Task<T>` 코루틴 인프라를 이미 갖추고 있음에도 활용하지 않고 있습니다. MVVM의 핵심인 **비동기 초기화 패턴**이 적용되지 않았습니다.

**수정**: `Initialize()`를 `Task<Expected<void>> InitializeAsync()`로 전환하고, `ITaskScheduler`를 통해 백그라운드에서 실행 후 결과를 `IPropertyChanged`로 통보.

***

## 🟡 개선 권장

### 9. `IniParser` — Adapter 내부 구현이 `IConfigRepository`를 우회

`IniConfigRepository::LoadConfig()`가 내부적으로 `IniParser`를 **직접 `new` 없이 스택에 생성**합니다. `IniParser`를 `IParser` 인터페이스로 추상화하면 파서 교체 및 단위 테스트가 용이해집니다. 현재는 직접 결합로 테스트 시 파서 모킹이 불가합니다.

***

### 10. `ErrorCategory::IO` 누락

`Win32FileCopyService`에서 `ErrorCategory::IO`를 사용하고 있으나, 현재 `ErrorCategory` enum에는 `IO` 항목이 없고 `Volume`, `Disk` 등만 있습니다. 정의되지 않은 enum 값 사용은 컴파일러에 따라 경고/오류가 됩니다.

***

## 📊 위반 사항 요약

| 번호 | 파일 | 위반 종류 | 심각도 |
|---|---|---|---|
| 1 | `Win32VolumeService.h` | 헤더에 `Windows.h` 노출 | 🔴 심각 |
| 2 | `TypeSelectorGroup.h` | UI → Domain 직접 의존 | 🔴 심각 |
| 3 | `StatusPanel.h` / `IWidget` | 추상 인터페이스에 Win32 타입 노출 | 🔴 심각 |
| 4 | `AnalysisRepository` 위치 | Persistence에 인메모리 저장소 혼재 | 🟠 경고 |
| 5 | `DIContainer` | `shared_ptr<void>` 타입 안전 우회 | 🟠 경고 |
| 6 | `Win32FileCopyService` | `WorkerContext` raw pointer 수명 오류 | 🟠 경고 |
| 7 | `AsyncIOCTL` | DIP 위반 — `IThreadPool` 미주입 | 🟠 경고 |
| 8 | `MainViewModel` | UI 스레드 블로킹 동기 초기화 | 🟠 경고 |
| 9 | `IniParser` | 인터페이스 미추상화 | 🟡 개선 |
| 10 | `ErrorCategory::IO` | 미정의 enum 값 사용 | 🟡 개선 |