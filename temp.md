코드를 충분히 파악했습니다. 트레이드오프 없이 순수하게 개선 가능한 지점들을 정리합니다.

***

## 성능 최적화 가능 지점 분석

코드 전반을 검토한 결과, **아키텍처/동작 변경 없이** 순수하게 성능만 개선 가능한 지점은 총 6곳입니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

***

## 1. `DIContainer::Resolve` — 불필요한 double-lock 경합

현재 Singleton fast-path에서 `std::shared_lock`으로 읽고, slow-path에서 `std::unique_lock`으로 업그레이드합니다.  문제는 Singleton 등록 시점(`RegisterInstance`)에 이미 `mSingletons`에 캐싱하므로, **팩토리가 있는 Singleton에만** slow-path가 발동합니다. 그런데 slow-path 진입 후 `mRegistrations`를 또 읽기 위해 이미 해제된 `readLock`을 다시 잡습니다. 이 과정에서 lock을 총 3번 획득합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** `Resolve` 내부에서 `mRegistrations`와 `mSingletons`를 같은 `readLock` 스코프 안에서 한 번에 조회하면 lock 획득 횟수가 2회 → 1~2회로 줄고, 특히 N번 호출 시 누적 효과가 큽니다.

***

## 2. `MFTScanner::BuildFilePathMap` — `GetFullPath` 반복 traversal

`BuildFilePathMap`에서 모든 레코드에 대해 `GetFullPath`를 호출하고, `GetFullPath` 내부에서는 `mFileRecordMap.find`를 깊이만큼 반복 호출합니다.  즉 파일 10만 개 기준 평균 깊이 5라면 **50만 번의 map lookup**이 발생합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** `BuildFilePathMap` 통과 시 top-down으로 부모 경로가 이미 계산된 경우 재사용하는 **메모이제이션(별도 `unordered_map<uint64_t, wstring>` pathCache)** 을 도입하면 lookup이 레코드 수 N에 선형으로 줄어듭니다.

```cpp
std::unordered_map<uint64_t, std::wstring> pathCache;

std::wstring MFTScanner::GetFullPath(uint64_t fileRefNumber) {
    if (auto it = pathCache.find(fileRefNumber); it != pathCache.end())
        return it->second;
    // ... 기존 traversal 로직 ...
    pathCache[fileRefNumber] = path;
    return path;
}
```

***

## 3. `MFTScanner::FindFilesByExtension` — 전체 맵 선형 탐색

현재 `FindFilesByExtension`은 `mFileRecordMap` 전체를 순회하며 확장자를 비교합니다.  ScanVolume 결과가 수십만 개일 경우 매 호출마다 O(N)입니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** `ScanVolume` 완료 후 `BuildFilePathMap` 단계에서 `unordered_map<wstring, vector<uint64_t>> mExtensionIndex`를 동시에 구축하면 `FindFilesByExtension`이 O(1) lookup으로 전환됩니다. 메모리 오버헤드는 확장자 문자열 키 추가분뿐이며, 동작은 동일합니다.

***

## 4. `SimpleButton` — `GetText` 호출 시 매번 Win32 버퍼 할당

`DrawButton` 내부에서 `GetText()`를 호출하고, `GetText`는 매번 `std::vector<wchar_t>` 버퍼를 heap에 할당 후 `GetWindowTextW`로 채웁니다.  `DrawButton`은 `WM_PAINT`마다 호출되므로 빈번한 heap allocation이 발생합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** `mText` 멤버 변수로 텍스트를 캐싱하고, `SetText`에서만 갱신하면 됩니다. 이미 `InvalidateCache`와 연동되어 있으므로 캐싱 로직 추가가 자연스럽습니다.

```cpp
// SetText에서
mText = text;
SetWindowTextW(mHwnd, text.c_str());
InvalidateCache();

// DrawButton에서
const std::wstring& displayText = mText;
```

***

## 5. `TextWidget::DrawBorder` — 매 Draw마다 `CreatePen` / `DeleteObject`

`Draw` 호출 시 border 라인 하나당 `CreatePen` → `SelectObject` → `LineTo` → `DeleteObject`를 반복합니다.  최대 4개 border면 4번의 GDI 객체 생성·소멸입니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** `TextWidgetStyle`이 변경될 때만 pen을 재생성해서 `mBorderPen` 멤버로 캐싱합니다. `SetStyle`은 이미 `mFontDirty` 플래그를 갖고 있으므로 동일한 패턴으로 `mPenDirty` 플래그를 추가하면 됩니다. `EnsureFont`와 완전히 동일한 구조로 확장 가능합니다.

***

## 6. `Win32Logger` — `std::source_location` 매개변수를 value로 전달

`ILogger::Log` 시그니처가 `std::source_location location` 를 **값으로** 받고 있습니다.  `std::source_location`은 내부적으로 4개의 포인터/정수를 보유하며, 모든 `Trace`, `Debug`, `Info` 등 래퍼 메서드가 이를 다시 `Log`로 전달할 때 복사가 발생합니다. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/157365209/e274b2cb-70ed-4312-b0d8-71cad642f01b/merged_codebase.md)

**개선:** 파라미터를 `const std::source_location&`로 변경합니다. `std::source_location`은 trivially copyable이라 실제 비용 차이는 크지 않지만, 호출 체인이 깊어질수록(`Trace` → `Log` → `LogEntry 생성`) 복사 횟수를 줄이는 것이 올바른 의도를 표현합니다.

***

## 요약

| 위치 | 문제 | 개선 방식 | 효과 |
|---|---|---|---|
| `DIContainer::Resolve` | lock 3회 획득 | 같은 스코프에서 단일 readLock으로 통합 | N번 Resolve 시 lock contention 감소 |
| `MFTScanner::GetFullPath` | 반복 map traversal | pathCache 메모이제이션 | O(N·depth) → O(N) |
| `MFTScanner::FindFilesByExtension` | 전체 맵 선형 탐색 | 확장자 역인덱스 구축 | O(N) → O(1) |
| `SimpleButton::DrawButton` | 매 Paint마다 `GetWindowTextW` heap alloc | `mText` 멤버 캐싱 | GDI/heap 비용 제거 |
| `TextWidget::DrawBorder` | 매 Draw마다 `CreatePen`/`DeleteObject` | pen 캐싱 + dirty 플래그 | GDI 생성 횟수 감소 |
| `Win32Logger` `source_location` | 불필요한 값 복사 | `const&`로 변경 | 복사 횟수 감소 |