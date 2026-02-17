충분히 깊게 검토했습니다. 정밀 분석 결과를 정리합니다.

***

# 코드 검토 결과 — 클린 아키텍처 위반 및 개선사항

## 🔴 심각 — 클린 아키텍처 위반

### 1. `LoadConfigurationUseCase` — 어댑터 직접 참조 (DIP 위반)

**가장 큰 위반.** Application 계층의 UseCase가 Adapters 계층의 구체 구현을 직접 `#include`하고 있습니다.

```cpp
// application/usecases/system/LoadConfigurationUseCase.cpp
#include <adapters/persistence/config/IniConfigRepository.h>  // ❌ 직접 참조
...
adapters::persistence::IniConfigRepository repository;  // ❌ 로컬 생성, DI 우회
auto result = repository.LoadConfig(configPath);
```

- **위반 원칙**: Application → Adapters 방향 의존 (의존성 역전 원칙 위반)
- **올바른 설계**: `IConfigRepository` 인터페이스를 생성자 주입받아야 함
- **추가 문제**: `IConfigRepository.h` 파일이 **본문이 비어있음** (인터페이스 자체가 미구현)

```cpp
// ✅ 올바른 구조
class LoadConfigurationUseCase {
public:
    explicit LoadConfigurationUseCase(
        std::shared_ptr<abstractions::IConfigRepository> repo,  // 인터페이스 주입
        std::shared_ptr<abstractions::ILogger> logger
    );
};
```

***

### 2. `MainViewModel` — UseCase 없이 Config 직접 로드 (레이어 스킵)

```cpp
// application/viewmodels/MainViewModel.cpp
domain::Expected<void> MainViewModel::LoadConfiguration() {
    // LoadConfigurationUseCase를 거치지 않고
    // IniConfigRepository를 직접 사용 가능성
}
```

`MainViewModel`이 `LoadConfigurationUseCase`를 생성자 주입받지 않고, 내부에서 직접 로직을 처리하는 구조입니다. ViewModel은 UseCase를 호출하는 역할만 해야 합니다.

***

### 3. `Win32MainWindow` — `IWindow` 인터페이스 시그니처 불일치

```cpp
// abstractions/ui/IWindow.h
virtual bool Create(void* hInstance, int nCmdShow) = 0;  // void* 사용 ❌
virtual void* GetHandle() const noexcept = 0;             // void* 반환 ❌

// Win32MainWindow.cpp (실제 구현)
bool Create(void* hInstance, int nCmdShow) override;
void* GetHandle() const noexcept override { return mHwnd; }
```

- **위반 원칙**: 인터페이스 자체에 `void*` 사용 — 계획서의 **"`void*` 금지"** 원칙 직접 위반
- **여파**: 인터페이스를 통해 받은 핸들을 사용하는 쪽에서 캐스팅 필요 → 타입 안전성 붕괴
- **해결 방향**: `Create()`의 `hInstance`를 추상화하거나, 플랫폼 종속적 `Create`는 인터페이스에서 제거하고 팩토리 패턴으로 위임

***

### 4. `SMBIOSParser.h` — Adapters 계층에 `Windows.h` 직접 노출

```cpp
// adapters/platform/win32/system/SMBIOSParser.h
#pragma once
#include <domain/primitives/Expected.h>
#include <Windows.h>  // ❌ 헤더에 노출
#include <string>
```

헤더에 `Windows.h`가 포함되어 있어, `SMBIOSParser.h`를 include하는 모든 파일에 Win32 타입이 오염됩니다. `.cpp` 구현 파일에만 포함해야 합니다.

***

### 5. `ServiceRegistration` — DI 등록 로직 내 순서 결합도 문제

```cpp
// main/ServiceRegistration.cpp
void ServiceRegistration::RegisterApplicationServices(DIContainer& container) {
    auto loggerResult = container.Resolve<abstractions::ILogger>();  // ❌ 수동 Resolve
    if (loggerResult.HasValue()) {
        auto logger = loggerResult.Value();
        auto viewModel = std::make_shared<application::MainViewModel>(logger);
        container.RegisterInstance<abstractions::IMainViewModel>(viewModel);
    }
    // ❌ IConfigRepository가 등록되지 않았기 때문에 LoadConfigurationUseCase에 주입 불가
}
```

`RegisterPlatformServices`, `RegisterUIServices` 본문이 비어있어 `IConfigRepository`, `Win32SystemInfoService` 등이 컨테이너에 등록되지 않는 상태입니다.

***

## 🟠 중요 — 코드 품질 문제

### 6. `DIContainer` — double-checked locking 패턴 버그

```cpp
// application/core/DIContainer.h (Resolve 구현)
{
    std::shared_lock readLock(mMutex);
    auto singletonIt = mSingletons.find(typeIndex);
    if (singletonIt != mSingletons.end())
        return ...; // 빠른 경로
}
// ❌ 여기서 락 해제 → 다른 스레드 진입 가능
{
    std::unique_lock writeLock(mMutex);
    auto singletonIt = mSingletons.find(typeIndex); // 재확인 없음
    auto instance = factory();
    mSingletons[typeIndex] = instance; // 중복 생성 가능
}
```

`shared_lock` 해제 후 `unique_lock` 획득 사이의 **TOCTOU(Time-of-check/time-of-use)** 구간에서 Singleton이 중복 생성될 수 있습니다. `unique_lock` 진입 후 **반드시 재확인**이 필요합니다.

```cpp
// ✅ 수정
std::unique_lock writeLock(mMutex);
auto singletonIt = mSingletons.find(typeIndex);  // 재확인
if (singletonIt != mSingletons.end())
    return std::static_pointer_cast<TInterface>(singletonIt->second);
auto instance = factory();
mSingletons[typeIndex] = instance;
```

***

### 7. `SMBIOSParser` — raw pointer + `new/delete` RAII 미적용

```cpp
// SMBIOSParser.cpp
mRawData = new(std::nothrow) BYTE[bufferSize];  // ❌ raw new
...
delete[] mRawData;  // ❌ raw delete (소멸자에서 수동 관리)
mRawData = nullptr;
```

예외나 early return 발생 시 메모리 누수 가능성이 있으며, 계획서의 **RAII 강제** 원칙을 위반합니다. `UniqueHandle`이나 `std::vector<BYTE>` / `std::unique_ptr<BYTE[]>`로 교체해야 합니다.

```cpp
// ✅ 수정
std::vector<BYTE> mRawData;  // RAII 자동 관리
```

***

### 8. `Win32MainWindow::DrawStatusText` — 매 Paint마다 폰트 생성/삭제

```cpp
void Win32MainWindow::DrawStatusText(HDC hdc) {
    HFONT hFont = CreateFontW(18, 0, 0, 0, ...);  // ❌ WM_PAINT마다 생성
    ...
    DeleteObject(hFont);  // ❌ WM_PAINT마다 삭제
}
```

`WM_PAINT`는 고빈도 이벤트입니다. `SimpleButton`/`ToggleButton`은 캐시를 구현했음에도 `Win32MainWindow`는 동일한 최적화가 없어 일관성이 없습니다. 폰트를 멤버 변수로 캐싱해야 합니다.

```cpp
// ✅ 멤버로 캐싱
platform::UniqueHandle mStatusFont;  // 생성자에서 1회 생성
```

***

### 9. `Win32SystemInfoService::GetMotherboardModel` — 에러를 삼키고 fallback 반환

```cpp
domain::Expected<std::wstring> Win32SystemInfoService::GetMotherboardModel() {
    auto initResult = EnsureSMBIOSInitialized();
    if (!initResult.HasValue()) {
        mLogger->Warning(L"SMBIOS not available...");
        return std::wstring(L"Unknown Motherboard");  // ❌ 에러를 성공으로 위장
    }
}
```

`Expected<T>`를 반환하면서 실패 시 **에러 대신 fallback 값을 성공으로 반환**합니다. 호출자가 실패 여부를 알 수 없게 됩니다. `GetEstimatedTime()` 같은 config 시간 조회가 이 값에 의존하므로 잘못된 마더보드 모델로 매핑될 수 있습니다.

***

### 10. `ToggleButton` / `SimpleButton` — `static std::unordered_map` 전역 상태

```cpp
// SimpleButton.h / ToggleButton.h
static std::unordered_map<HWND, SimpleButton*> sInstances;  // ❌ 전역 상태
static std::unordered_map<int, std::vector<ToggleButton*>> sGroups;
```

- 멀티스레드 환경에서 `sInstances`/`sGroups` 접근에 **락이 없음**
- 포인터(`*`) 보관으로 댕글링 포인터 위험
- 테스트 간 상태 오염 가능성

`std::mutex` 보호 또는 `shared_ptr` + `weak_ptr` 패턴으로 교체가 필요합니다.

***

## 🟡 개선 권고 — 설계 일관성

| 항목 | 현재 | 개선 방향 |
|---|---|---|
| `IConfigRepository.h` 본문 비어있음 | 스텁 파일만 존재 | `LoadConfig()` 인터페이스 정의 필요 |
| `Win32Logger` 로그 파일 경로 하드코딩 | `L"log/log.txt"` 고정 | 생성자 파라미터로 분리 |
| `IniParser` namespace-free 헬퍼 함수 | `GetStringFromTable()` 등이 익명 namespace 안 free function | `private` static 멤버 또는 별도 유틸 클래스로 이동 |
| `Win32SystemInfoService::mSmbiosInitialized` | `bool` 단순 플래그 | `std::once_flag` + `std::call_once` 사용 권장 (멀티스레드 안전) |
| `EventBus` 구독 해제 | `SubscriptionToken` 기반이지만 RAII 구독 래퍼 없음 | `ScopedSubscription` RAII 래퍼 추가 권장 |

***

## 우선순위 요약

```
🔴 즉시 수정
  1. LoadConfigurationUseCase → IConfigRepository 인터페이스 정의 및 DI 주입
  2. IWindow::Create/GetHandle → void* 제거
  3. SMBIOSParser → raw new/delete → std::vector<BYTE> 교체

🟠 다음 기능 작업 전 수정
  4. DIContainer → double-checked locking 재확인 로직 추가
  5. Win32SystemInfoService → fallback 에러 처리 방식 수정
  6. Win32MainWindow → DrawStatusText 폰트 캐싱

🟡 장기 개선
  7. SMBIOSParser.h → Windows.h를 .cpp로 이동
  8. ToggleButton/SimpleButton → static map 스레드 안전화
  9. ServiceRegistration → IConfigRepository 등록 추가
```