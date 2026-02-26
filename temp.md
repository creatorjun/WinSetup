코드베이스 전체를 파악했습니다. 현재 완성된 Phase(1~8)를 기준으로 최적화 가능한 항목들을 분석합니다.

***

## 최적화 검토 결과

현재 코드는 전반적으로 잘 설계되어 있습니다. 다만 **3개 등급**으로 분류해 실질적으로 개선 가능한 항목들을 정리했습니다.

***

## 🔴 High — 버그 유발 가능성 있음

### 1. `FindDiskIndexByVolumeGuid` 중복 호출

`AnalyzeVolumesUseCase::Execute()`에서 같은 GUID에 대해 `FindDiskIndexByVolumeGuid`를 **최대 3회** 호출합니다. 이 함수는 내부적으로 `CreateFile` + `DeviceIoControl`을 수행하는 **무거운 Win32 호출**입니다.

```cpp
// systemVolIdx 탐색 시 → systemGuid로 1회
const auto systemDiskIdx = mPathChecker->FindDiskIndexByVolumeGuid(systemGuid);

// Boot 볼륨 탐색 루프에서 각 볼륨마다 → 중복 1회
const auto volDiskIdx = mPathChecker->FindDiskIndexByVolumeGuid(vol.GetVolumePath());

// Data 볼륨 탐색 시 → 또 1회
const auto dataDiskIdx = mPathChecker->FindDiskIndexByVolumeGuid(dataVolIt->GetVolumePath());
```

**개선:** `Execute()` 진입 시 볼륨 목록을 순회하며 `std::unordered_map<std::wstring, std::optional<uint32_t>>`로 한 번에 캐싱.

***

### 2. `Win32DiskService::FormatPartition` — NTFS 전용 IOCTL 후 무조건 cmd.exe 폴백

현재 구조는 `FSCTL_FORMAT_VOLUME`이 실패하면 `ShellExecuteExW`로 `cmd.exe format`을 호출합니다. WinPE 환경에서 `ShellExecuteExW`는 셸이 없거나 UAC 제한으로 **silent fail** 가능성이 있고, `WaitForSingleObject` 타임아웃(60초)이 고정되어 있어 대용량 파티션에서 오탐이 발생합니다.

***

## 🟡 Medium — 성능 또는 안정성 개선

### 3. `EnumerateDiskIndicesViaSetupAPI` — SetupAPI 대신 `\\.\PhysicalDriveN` 순차 탐색 병용 없음

현재는 SetupAPI만 사용하므로 일부 WinPE 환경에서 `GUID_DEVINTERFACE_DISK` 열거가 불완전할 수 있습니다. 로그에서 `diskIdx=NOT FOUND`가 발생했던 근본 원인도 이와 무관하지 않습니다.

**개선 (방어 로직 추가):** SetupAPI 결과가 0개이면 `PhysicalDrive0`~`PhysicalDrive15` 순차 탐색으로 폴백.

***

### 4. `IniParser` — `IniData`가 `std::vector<pair>` 구조

현재 `IniData = std::vector<SectionEntry>`이고, `FindSection`은 선형 탐색(`O(N)`)입니다. 섹션 수가 많지 않아 현재는 문제없지만, `FindValue` 역시 선형이라 키가 많은 `[BACKUP]` 섹션에서는 `O(N*M)` 탐색이 발생합니다.

**개선:** `std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>`으로 교체. 단, 섹션/키 **순서 보존**이 필요한 경우 유지.

***

### 5. `Win32PathChecker::FindDiskIndexByVolumeGuid` — `FILE_SHARE_READ | FILE_SHARE_WRITE` 없이 실패 시 재시도 없음

현재 `CreateFileW`에 공유 플래그는 올바르게 설정되어 있지만, `IOCTL_STORAGE_GET_DEVICE_NUMBER` 실패 시 즉시 `nullopt`를 반환합니다. 일부 WinPE 환경에서는 볼륨이 초기화 중일 때 첫 번째 호출이 실패할 수 있습니다.

***

### 6. `AnalyzeVolumesUseCase::LogResult` — `GetVolumes()` 재호출

`Execute()` 내에서 이미 로컬 `volumes` 벡터를 가지고 있음에도 `LogResult()`에서 `mAnalysisRepository->GetVolumes()`를 **다시 호출**합니다. `StoreUpdatedVolumes` 이후이므로 데이터는 동일하지만, 불필요한 `shared_ptr` 역참조가 발생합니다.

**개선:** `LogResult(const std::vector<domain::VolumeInfo>& volumes)`로 시그니처 변경해 로컬 벡터를 직접 전달.

***

## 🟢 Low — 코드 품질 개선

### 7. `PartitionType` 분기 switch문 4군데 중복

`AnalyzeVolumesUseCase.cpp`에서 방금 제거한 디버그 코드 외에도 `Win32DiskService.cpp`와 `GetCurrentLayout` 내에서 `PartitionType → GUID` 매핑 switch가 중복으로 작성되어 있습니다.

**개선:** `Win32TypeMapper`에 `ToGuid(PartitionType)` / `FromGuid(GUID) → PartitionType` 정적 메서드 추가. 현재 `Win32TypeMapper`에는 `BusType` 매핑만 존재합니다.

***

### 8. `DiskSize::ToBytes()` 미정의

`Win32DiskService.cpp`에서 `partition.GetSize().ToBytes()`를 호출하고 있는데, API 레퍼런스 기준으로 `DiskSize`에는 `GetBytes()`만 정의되어 있고 `ToBytes()`는 없습니다. 현재 빌드가 통과되고 있다면 `GetBytes()`의 별칭이거나 정의가 누락된 상태일 수 있어 확인이 필요합니다.

***

## 우선순위 요약

| 우선순위 | 항목 | 영향 |
|---|---|---|
| 🔴 1순위 | `FindDiskIndexByVolumeGuid` 캐싱 | 성능 + Win32 호출 최소화 |
| 🔴 2순위 | `FormatPartition` cmd.exe 폴백 개선 | WinPE 안정성 |
| 🟡 3순위 | `LogResult` 재호출 제거 | 코드 품질 |
| 🟡 4순위 | SetupAPI 폴백 로직 추가 | WinPE 호환성 |
| 🟢 5순위 | `Win32TypeMapper` GUID 매핑 통합 | 유지보수성 |
| 🟢 6순위 | `DiskSize::ToBytes()` 존재 여부 확인 | 잠재적 빌드 오류 |

어떤 항목부터 적용할까요?