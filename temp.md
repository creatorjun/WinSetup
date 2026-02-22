이제 전체 코드를 완전히 분석했습니다. 발견된 모든 위반사항을 정리합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/104418831/fac86264-42b9-4659-8b2e-fd6775d6725e/WinSetup_022300.txt)

***

# 클린 아키텍처 위반사항 전수 검토 보고서

## 총평

전체적으로 아키텍처 설계는 매우 훌륭하나, **구체 구현 세부에서 6가지 카테고리의 위반**이 발견되었습니다.

***

## 🔴 위반 1 — Domain에 `DiskInfo.h`가 `PartitionInfo.h`를 include

**파일**: `src/domain/entities/DiskInfo.h`
**심각도**: 높음

```cpp
// DiskInfo.h (Domain)
#include "PartitionInfo.h"  // ← 같은 Domain 내이므로 OK처럼 보이지만...

class DiskInfo {
    std::vector<PartitionInfo> mPartitions;  // ← 집합 관계로 인한 강결합
    void AddPartition(const PartitionInfo& partition);
};
```

**문제**: `DiskInfo`가 `PartitionInfo`를 직접 컨테이너로 가지면 두 엔티티가 강하게 결합됩니다. 클린 아키텍처에서 Entity 간 관계는 ID(값 객체) 참조로 느슨하게 유지해야 합니다.

**올바른 방향**: `DiskInfo`는 `PartitionId`(값 객체) 리스트만 가지고, `PartitionInfo` 조회는 Repository를 통해야 합니다. 또는 현실적으로 WinPE 도메인 특성상 집합 관계를 허용한다면, `DiskInfo`는 Aggregate Root로 명시적으로 선언하고 주석으로 설계 의도를 문서화해야 합니다.

***

## 🔴 위반 2 — `SystemInfo.h`가 `DiskInfo.h` / `VolumeInfo.h`를 포함

**파일**: `src/domain/entities/SystemInfo.h`
**심각도**: 높음

```cpp
// SystemInfo.h (Domain)
#include "DiskInfo.h"   // ← 다른 엔티티를 직접 임베드
#include "VolumeInfo.h" // ← 동일 문제

class SystemInfo {
    std::vector<DiskInfo>   mDisks;   // ← 전체 DiskInfo 그래프를 포함
    std::vector<VolumeInfo> mVolumes; // ← 전체 VolumeInfo 그래프를 포함
    void AddDisk(DiskInfo disk);
    void AddVolume(VolumeInfo vol);
};
```

**문제**: `SystemInfo`가 `DiskInfo`와 `VolumeInfo`를 직접 소유하면 세 엔티티 전체가 하나의 거대 오브젝트 그래프로 결합됩니다. `SystemInfo`의 책임은 시스템 메타정보(보드 모델, BIOS, UEFI)에만 한정되어야 합니다. 디스크/볼륨 정보는 별도 AnalysisResult DTO로 분리해야 합니다.

**올바른 방향**: `SystemInfo`에서 `mDisks`, `mVolumes` 제거 → `SystemAnalysisResult.h` DTO에 통합.

***

## 🔴 위반 3 — `DIContainer`에 `std::shared_ptr<void>` 사용

**파일**: `src/application/core/DIContainer.h`
**심각도**: 높음

```cpp
// DIContainer.h
std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;

auto factory = [this]() -> std::shared_ptr<void> {
    return std::make_shared<TImplementation>();
};
```

**문제**: `std::shared_ptr<void>`는 `void*`와 동일한 타입 안전성 문제를 가집니다. 계획서에서 **"void* 금지"를 명시**했음에도 불구하고 DI 컨테이너 내부에서 직접 위반하고 있습니다. 또한 `std::static_pointer_cast<TInterface>(instance)`로 다운캐스트 시 런타임 오류 위험이 있습니다.

**올바른 방향**: `std::any` 또는 명시적 타입 소거 패턴(type-erased wrapper) 사용:
```cpp
// 개선 방향
std::unordered_map<std::type_index, std::any> mSingletons;
// 꺼낼 때: std::any_cast<std::shared_ptr<TInterface>>(it->second)
```

***

## 🔴 위반 4 — `ServiceRegistration`에서 구체 타입 직접 참조

**파일**: `src/main/ServiceRegistration.cpp`
**심각도**: 중간

```cpp
// ServiceRegistration.cpp (Main Layer)
#include "adapters/persistence/config/IniConfigRepository.h"
#include "adapters/platform/win32/logging/Win32Logger.h"
#include "adapters/platform/win32/system/Win32SystemInfoService.h"
#include "adapters/ui/win32/Win32MainWindow.h"
#include "application/usecases/system/LoadConfigurationUseCase.h"
#include "application/viewmodels/MainViewModel.h"

// 직접 구체 타입으로 인스턴스 생성
auto logger = std::make_shared<adapters::platform::Win32Logger>(L"log/log.txt");
container.RegisterInstance<abstractions::ILogger>(
    std::static_pointer_cast<abstractions::ILogger>(logger)  // ← 명시적 캐스트 필요 자체가 냄새
);
```

**문제**: Main Layer(Composition Root)가 구체 타입을 아는 것은 **의도된 설계**이지만, 현재 구현에서 의존성 주입 실패 시 `return;`으로 조용히 실패하는 것이 문제입니다. 등록 실패가 무시되면 이후 `Resolve`에서 원인을 파악하기 어렵습니다.

```cpp
// 현재: 실패를 조용히 무시
if (!loggerResult.HasValue() || !repoResult.HasValue()) return;  // ← 위험

// 올바른 방향: 실패 시 명시적 패닉
auto result = container.Resolve<abstractions::ILogger>();
if (!result.HasValue()) {
    throw std::runtime_error("Critical: ILogger registration failed");
}
```

***

## 🟡 위반 5 — `StatusPanel`/`ActionPanel`에서 매번 `CreateFont` 호출 (리소스 누수)

**파일**: `src/adapters/ui/win32/panels/StatusPanel.cpp`, `ActionPanel.cpp`
**심각도**: 중간

```cpp
// StatusPanel.cpp - OnPaint마다 실행됨
void StatusPanel::DrawStatusText(HDC hdc) const {
    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, ...);  // ← WM_PAINT마다 생성
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
    // ... 그리기 ...
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);  // 삭제는 하지만...

    HFONT hFont2 = CreateFontW(14, 0, 0, 0, ...);  // ← 같은 함수 내 또 생성
    // ...
    DeleteObject(hFont2);
}
```

**문제**: `WM_PAINT`는 매우 빈번하게 발생합니다. `CreateFont`/`DeleteObject`를 매 페인트 사이클마다 반복하면 GDI 리소스 압박이 생깁니다. RAII 원칙에도 맞지 않습니다.

**올바른 방향**: 멤버 변수로 `HFONT`를 캐싱하거나 `UniqueHandle` 패턴을 GDI 객체에도 적용:
```cpp
// 개선 방향 — 멤버로 캐싱
class StatusPanel {
    mutable HFONT mStatusFont = nullptr;   // 생성자/Create에서 1회 생성
    mutable HFONT mDescFont   = nullptr;
    // 소멸자에서 DeleteObject
};
```

***

## 🟡 위반 6 — `OptionPanel`/`ActionPanel`의 멤버 변수명 불일치 (코딩 스타일 위반)

**파일**: `src/adapters/ui/win32/panels/OptionPanel.cpp`, `OptionPanel.h`
**심각도**: 낮음

```cpp
// OptionPanel.h
class OptionPanel {
    std::shared_ptr<abstractions::IMainViewModel> mviewModel;  // ← 소문자 'm'
    ToggleButton mbtnDataPreserve;                              // ← 소문자 'm'
    ToggleButton mbtnBitlocker;                                 // ← 소문자 'm'
};

// ActionPanel.h — 같은 프로젝트
class ActionPanel {
    std::shared_ptr<abstractions::IMainViewModel> mViewModel;  // ← 대문자 'V'
    SimpleButton mBtnStartStop;                                 // ← 대문자 'B'
};
```

**문제**: 프로젝트 전반의 멤버 변수 네이밍 컨벤션(`mXxx`)을 `OptionPanel`만 `mxxx`로 어기고 있습니다. 일관성 없는 스타일은 유지보수 시 혼란을 유발합니다.

***

## 🟡 위반 7 — `DomainEvent`와 `IEvent` 계층 중복

**파일**: `src/domain/events/DomainEvent.h` vs `src/abstractions/infrastructure/messaging/IEvent.h`
**심각도**: 중간

```cpp
// abstractions/infrastructure/messaging/IEvent.h
class IEvent {  // ← Abstractions 계층
    virtual std::wstring GetEventType() const noexcept = 0;
    virtual std::type_index GetTypeIndex() const noexcept = 0;
};

// domain/events/DomainEvent.h
class DomainEvent {  // ← Domain 계층
    virtual std::wstring GetEventType() const noexcept = 0;  // ← 동일 메서드
    uint64_t GetEventId() const noexcept;
};
```

**문제**: `DomainEvent`와 `IEvent`가 독립적으로 존재하며, `DiskAnalyzedEvent`는 `DomainEvent`를 상속하지만 `IEvent`를 상속하지 않습니다. 따라서 `IEventBus::Publish()`에 도메인 이벤트를 전달할 수 없습니다. 두 계층의 이벤트 시스템이 서로 연결되지 않아 **이벤트 버스가 사실상 무용지물**입니다.

**올바른 방향**: `DomainEvent`가 `abstractions::IEvent`를 상속하거나, Domain 이벤트를 Adapter에서 `IEvent`로 래핑하는 변환 계층을 추가해야 합니다.

***

## 위반 요약표

| # | 파일 | 위반 유형 | 심각도 |
|---|---|---|---|
| 1 | `domain/entities/DiskInfo.h` | Entity 간 직접 임베딩 (Aggregate 미선언) | 🔴 높음 |
| 2 | `domain/entities/SystemInfo.h` | Entity 간 직접 임베딩 (책임 과부하) | 🔴 높음 |
| 3 | `application/core/DIContainer.h` | `shared_ptr<void>` 사용 (void* 금지 위반) | 🔴 높음 |
| 4 | `main/ServiceRegistration.cpp` | DI 실패 시 조용한 무시 (안정성 위반) | 🔴 중간 |
| 5 | `adapters/ui/win32/panels/StatusPanel.cpp` | WM_PAINT마다 GDI 객체 생성 (RAII 미적용) | 🟡 중간 |
| 6 | `adapters/ui/win32/panels/OptionPanel.h` | 멤버 변수 네이밍 컨벤션 불일치 | 🟡 낮음 |
| 7 | `domain/events/` vs `abstractions/messaging/` | 이벤트 계층 이중화, 연결 누락 | 🟡 중간 |

 [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/104418831/fac86264-42b9-4659-8b2e-fd6775d6725e/WinSetup_022300.txt)

***

수정 우선순위는 **3번(DIContainer) → 2번(SystemInfo) → 7번(이벤트 연결) → 1번(DiskInfo Aggregate)** 순서를 권장합니다. 수정 요청하시면 클린 아키텍처 원칙에 맞게 전체 파일 단위로 제공하겠습니다.