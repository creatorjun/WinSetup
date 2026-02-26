전체 코드를 충분히 파악했습니다. 이제 분석 결과를 정리하겠습니다.

***

코드베이스 전체를 처음부터 끝까지 꼼꼼히 검토했습니다. 클린 아키텍처(Domain → Abstractions → Application → Adapters)를 매우 일관되게 지키고 있고, RAII, `Expected<T>`, `UniqueHandle` 등 코드 품질도 전반적으로 높습니다. 그러나 성능 관점에서 개선할 수 있는 포인트를 우선순위별로 정리합니다.

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

***

## P0 — 즉시 수정 (버그성 성능 이슈)

### 1. `Win32ThreadPoolExecutor` — Single Event, Thundering Herd
현재 `mWakeEvent`가 **auto-reset** 이벤트임에도 불구하고, `WorkerLoop` 안에서 `WaitForSingleObject` 한 번에 **한 스레드만 깨어난 뒤** 나머지 태스크가 있으면 `SetEvent`를 재호출하는 방식입니다.  스레드 수 × 태스크 수만큼 `SetEvent`가 연쇄 호출되어 **컨텍스트 스위치 폭풍**이 발생할 수 있습니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:** 스레드당 개별 wake event를 두거나, `IOCP(CreateIoCompletionPort + PostQueuedCompletionStatus)`로 교체하면 커널 단에서 1:1 dispatch가 보장됩니다.

### 2. `AsyncIOCTL::ProcessOperation` — 블로킹 wait를 Executor 스레드 안에서 실행
```cpp
// 현재 코드 — Executor Worker 스레드 안에서 INFINITE wait
WaitForSingleObject(op->overlapped.hEvent, INFINITE);
```
Executor 스레드가 IOCTL 완료까지 **blocking** 상태가 됩니다.  스레드 풀 전체가 I/O 대기에 잠식되면 CPU-bound 태스크가 기아(starvation) 상태에 빠집니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:** `DeviceIoControl`을 진짜 overlapped I/O로 발행하고 IOCP에 등록한 후, 완료 콜백에서 후처리하는 구조로 분리해야 합니다.

***

## P1 — 구조적 성능 이슈

### 3. `Win32Logger` — Log() 호출마다 Lock + 매 호출 시 `wstring` 조립
```cpp
// 현재: Log() 호출 시마다 stdlock_guard + entry 문자열을 즉시 조립
stdlock_guard lock(mMutex);
...
WriteBufferedEntry(entry, forceFlush);
```
`entry.reserve(256)` 이후 `+=` 연산으로 문자열을 이어붙이고 있는데, 이 과정이 **모든 Log() 호출마다 lock 경쟁** 상황에서 발생합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:**
- 생산자는 `lock-free queue(moodycamel::ConcurrentQueue 등)`에 `push`만 수행
- 별도 Writer 스레드가 배치(batch)로 파일에 기록
- 문자열 조립은 Writer 스레드에서만 수행 (lock 경쟁 제거)

### 4. `DIContainer::Resolve` — Transient는 매번 write-lock 획득
```cpp
// Singleton fast-path는 shared_lock이지만,
// Transient는 unique_lock(write lock)으로 업그레이드 없이 바로 진입
stduniquelock writelock(mMutex); // <- Transient 경우
```
`shared_mutex`를 사용하고 있음에도, Transient 서비스 해결 경로가 write-lock을 잡고 있어 Singleton fast-path의 `shared_lock` 장점이 희석됩니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:** Transient는 factory만 꺼내오는 데 read-lock으로 충분하므로, factory 함수를 `shared_lock` 하에 복사한 뒤 lock 해제 후 인스턴스를 생성합니다.

### 5. `Win32FileCopyService::CopySingleFile` — 파일 복사에 `FILE_FLAG_NO_BUFFERING` 부재
소스 파일 open 시 `FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING`을 사용하고 있는데, `FILE_FLAG_NO_BUFFERING`은 **섹터 정렬 버퍼**를 요구합니다.  [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md) 현재 `std::vector<BYTE>`로 힙 할당한 버퍼는 섹터 정렬이 보장되지 않아 런타임에 `ERROR_INVALID_PARAMETER`가 발생하거나 내부적으로 버퍼링이 fallback되어 성능 이득이 없어집니다.

**해결 방향:** `_aligned_malloc(bufSize, 512)`로 VirtualAlloc 기반 정렬 버퍼를 사용하거나, `FILE_FLAG_NO_BUFFERING`을 제거하고 `FILE_FLAG_SEQUENTIAL_SCAN`만 유지합니다.

***

## P2 — 최적화 기회

### 6. `Win32StringHelper` — `swprintf_s` 매번 스택 버퍼 → `std::wstring` 복사
```cpp
static std::wstring UInt64ToString(uint64_t value) {
    wchar_t buffer[32];
    swprintf_s(buffer, 32, L"%llu", value);
    return std::wstring(buffer);  // 복사 발생
}
```
Log hot-path에서 숫자 → 문자열 변환이 빈번한 경우, `std::to_wstring()` 또는 `std::format()`(C++20)이 더 효율적이고 코드도 간결해집니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

### 7. `Win32ProgressBar::DrawProgress` — `CreateSolidBrush / DeleteObject` 매 WM_PAINT 호출
```cpp
HBRUSH hBgBrush = CreateSolidBrush(COLOR_TRACK);
FillRect(memDC, &rc, hBgBrush);
DeleteObject(hBgBrush);
// ... 이후 hFill, hPen도 동일 패턴
```
`WM_PAINT`마다 GDI 오브젝트를 생성/파괴합니다.  `RenderCache`가 이미 있음에도 브러시/펜은 캐싱하지 않고 있습니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:** 색상이 고정값(`constexpr COLORREF`)이므로 브러시와 펜을 멤버 `UniqueHandle`로 한 번만 생성하고 재사용합니다.

### 8. `AsyncIOCTL::FindOperation / RemoveOperation` — 선형 탐색 O(N)
```cpp
auto it = std::find_if(mOperations.begin(), mOperations.end(),
    [operationId](auto& op) { return op->id == operationId; });
```
`mOperations`이 `std::vector`이므로 ID 기반 조회가 O(N)입니다.  동시 작업이 많아질수록 병목이 됩니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/ace7b569-44d5-4768-8e37-0000c3bba75d/merged_codebase.md)

**해결 방향:** `std::unordered_map<uint32_t, std::shared_ptr<AsyncOperation>>` 으로 교체하면 O(1) 조회가 가능합니다.

***

## 전체 요약

| 우선순위 | 항목 | 영향 범위 |
|---|---|---|
| P0 | `Win32ThreadPoolExecutor` Thundering Herd | CPU 전체 |
| P0 | `AsyncIOCTL` Executor 스레드 블로킹 | I/O 처리량 |
| P1 | `Win32Logger` Lock contention | 모든 로그 경로 |
| P1 | `DIContainer` Transient write-lock | 서비스 해결 전반 |
| P1 | `Win32FileCopyService` 버퍼 미정렬 | 파일 복사 안정성/성능 |
| P2 | `Win32StringHelper` 문자열 변환 | Log hot-path |
| P2 | `Win32ProgressBar` GDI 오브젝트 매번 생성 | UI 렌더링 |
| P2 | `AsyncIOCTL` 선형 탐색 O(N) | 다중 IOCTL 동시 발행 시 |

P0 두 항목은 실제 처리량과 직결되는 구조적 문제이므로 우선 수정을 권장합니다. 어느 항목부터 코드 수정을 시작할지 말씀해 주시면 바로 진행하겠습니다.
