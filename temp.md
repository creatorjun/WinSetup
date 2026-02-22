이제 전체 코드를 완전히 분석했습니다. 발견된 모든 위반사항을 정리합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/104418831/fac86264-42b9-4659-8b2e-fd6775d6725e/WinSetup_022300.txt)

***

# 클린 아키텍처 위반사항 전수 검토 보고서

## 총평

전체적으로 아키텍처 설계는 매우 훌륭하나, **구체 구현 세부에서 6가지 카테고리의 위반**이 발견되었습니다.

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