코드베이스를 충분히 파악했습니다. 이제 분석 결과를 정리합니다.

***

## 클린 아키텍처 분석 - WinSetup (AnalyzeSystemUseCase 완성 기준)

전체 레이어 구조(Domain → Abstractions → Application → Adapters → Main)는 명확히 설계되어 있으며 의존성 방향도 잘 지켜지고 있습니다. 다만 현재 구현 범위 내에서 몇 가지 개선이 필요한 구체적 문제들이 존재합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

***

## 1. `AnalyzeSystemUseCase` 내 책임 혼재 (SRP 위반)

**문제:** `AnalyzeSystemUseCase::Execute()`가 직접 `ILoadConfigurationUseCase`를 호출하여 설정을 로드하고, 그 결과에서 예상 시간을 조회하는 로직까지 포함하고 있습니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

```cpp
// AnalyzeSystemUseCase::Execute() 내부
auto configResult = mLoadConfiguration->Execute(L"config.ini");
const auto times = configResult.Value->GetEstimatedTimes();
const auto it = times.find(boardModel);
```

UseCase가 다른 UseCase를 직접 내부에서 호출하는 것은 UseCase 간 암묵적 순서 의존성과 결합도를 만듭니다. `AnalyzeSystemUseCase`의 책임은 순수하게 **시스템 하드웨어 분석**이어야 합니다. 예상 시간 조회는 `MainViewModel::RunInitializeOnBackground()`에서 두 UseCase를 독립적으로 순서대로 호출하는 방식으로 분리해야 합니다.

**개선 방향:**
- `AnalyzeSystemUseCase`에서 `ILoadConfigurationUseCase` 의존성 제거
- 예상 시간 로직은 ViewModel 또는 별도의 Orchestrator로 이동

***

## 2. `AnalysisRepository` 위치 오류 (레이어 위반)

**문제:** `AnalysisRepository`의 헤더 경로가 `applicationrepositories/AnalysisRepository.h`인데, 구현 파일 namespace는 `winsetupadapterspersistence`입니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

```cpp
// .h: #include "applicationrepositories/AnalysisRepository.h"
// .cpp: namespace winsetupadapterspersistence { ... }
```

Application 레이어는 인터페이스만 소유해야 하며, 구체 구현은 Adapters 레이어에 있어야 합니다. 물리적 파일 경로(`src/adapters/persistence/analysis/`)와 헤더 include 경로가 불일치하면 레이어 경계가 코드에서 불명확해집니다.

**개선 방향:**
- `AnalysisRepository.h/.cpp`를 `src/adapters/persistence/analysis/`로 경로 통일
- `ServiceRegistration.cpp`의 include도 `adapters` 경로로 수정

***

## 3. Step 클래스의 Guard 패턴 비일관성

**문제:** `EnumerateDisksStep`과 `EnumerateVolumesStep`은 의존성이 없으면 빈 컨테이너를 반환(soft fail)하는 반면, `AnalyzeDisksStep`과 `AnalyzeVolumesStep`은 `domain::Error`를 반환(hard fail)합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

```cpp
// EnumerateDisksStep - soft fail
if (!mDiskService) { return std::make_shared<...>(); }

// AnalyzeDisksStep - hard fail
if (!mAnalysisRepository) { return domain::Error(..., ErrorCategory::System); }
```

같은 Step 계층 내에서 필수 의존성 누락에 대한 처리 정책이 달라 호출 측에서 동작을 예측하기 어렵습니다. `IDiskService` 없이 진행하는 것은 논리적으로도 불가능하므로 Enumerate 계열도 hard fail이 적합합니다.

**개선 방향:**
- 모든 Step의 필수 의존성 누락 시 `domain::Error` 반환으로 통일

***

## 4. `SetupConfig`의 `BitLockerPin` 도메인 오염

**문제:** `SetupConfig`(Domain 엔티티)가 `mBitLockerPin` 필드를 직접 보유하고 있습니다. BitLocker PIN은 보안 자격증명(Security Credential)으로, 도메인 엔티티가 이를 plain `std::wstring`으로 소유하는 것은 Domain 순수성을 해칩니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

**개선 방향:**
- `BitLockerPin`을 `std::wstring`이 아닌 별도 Value Object(`BitLockerPin` 또는 `SecurePin`)로 래핑
- 또는 `ConfigureBitLockerUseCase`가 config에서 PIN을 직접 읽도록 하고 `SetupConfig`에서 제거

***

## 5. `AnalyzeVolumesStep`의 Domain 로직 침범

**문제:** `IsSystemVolume()`, `IsDataVolume()`, `IsBootVolume()` 판별 로직이 Application 레이어의 Step 클래스 내부에 `private` 메서드로 구현되어 있습니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

```cpp
bool AnalyzeVolumesStep::IsSystemVolume(const domain::VolumeInfo& volume, ...) const noexcept {
    return mPathChecker->IsDirectory(guid, L"Windows\\System32") && ...;
}
```

볼륨이 시스템 볼륨인지 판별하는 규칙은 **도메인 지식**입니다. 이미 `DiskSpecifications`, `VolumeSpecifications`라는 Specification 클래스가 Domain 레이어에 존재하는데 이와 일관성이 없습니다.

**개선 방향:**
- `VolumeSpecifications`에 `SystemVolumeSpec`, `DataVolumeSpec`, `BootVolumeSpec` 추가
- Step은 Specification을 주입받아 사용하는 구조로 변경

***

## 6. `MainViewModel`의 UseCase 직접 실행 스레드 처리

**문제:** `MainViewModel::RunInitializeOnBackground()`가 `std::thread`를 직접 생성합니다. Abstractions 레이어에 `IThreadPool`, `IScheduler`, `IExecutor`가 이미 정의되어 있음에도 ViewModel이 직접 스레드를 생성하면, ViewModel 단위테스트 시 실제 스레드가 실행되는 문제가 발생합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

**개선 방향:**
- `MainViewModel`에 `IExecutor` 또는 `IScheduler`를 주입
- `Execute()`를 executor에 위임하여 테스트 시 동기 실행 가능하도록 구성

***

## 7. `Dispatcher`의 Win32 직접 의존 (Adapter 경계 누락)

**문제:** `Dispatcher` 클래스가 `application/services/` 경로에 위치하지만, `#include <Windows.h>`를 직접 포함하고 `PostMessageW`, `HWND` 등 Win32 타입을 사용합니다. Application 레이어는 플랫폼에 독립적이어야 합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/6337e85f-3559-48c0-8696-88a0a8d8f70c/merged_codebase.md)

**개선 방향:**
- `Dispatcher`를 `adapters/platform/win32/` 로 이동 (`Win32Dispatcher`로 rename)
- Application 레이어에는 `IUIDispatcher` 인터페이스만 잔류

***

## 요약

| 구분 | 위치 | 문제 유형 |
|------|------|-----------|
| UseCase→UseCase 직접 호출 | `AnalyzeSystemUseCase` | SRP / 결합도 |
| Repository 위치 불일치 | `AnalysisRepository` | 레이어 경계 |
| Step fail 정책 비일관성 | Enumerate계열 Step | 일관성 |
| BitLockerPin raw 보유 | `SetupConfig` | Domain 순수성 |
| 볼륨 판별 로직 위치 | `AnalyzeVolumesStep` | Domain 지식 누출 |
| ViewModel 직접 스레드 생성 | `MainViewModel` | 테스트 용이성 |
| Dispatcher Win32 직접 의존 | `Dispatcher` | 레이어 경계 |