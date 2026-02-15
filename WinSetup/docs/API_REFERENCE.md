# WinSetup API Reference v1.0

## 📑 목차

1. [Domain 계층 API](#domain-계층-api)
   - [Primitives](#primitives)
   - [Entities](#entities)
   - [Value Objects](#value-objects)
   - [Services](#services)
   - [Specifications](#specifications)

2. [Abstractions 계층 API](#abstractions-계층-api)
   - [Storage Services](#storage-services)
   - [Infrastructure Services](#infrastructure-services)
   - [UI Services](#ui-services)

3. [Application 계층 API](#application-계층-api)
   - [Core](#core)
   - [Use Cases](#use-cases)
   - [Async](#async)

4. [Adapters 계층 API](#adapters-계층-api)
   - [Win32 Platform](#win32-platform)
   - [Storage](#storage)
   - [Imaging](#imaging)

---

## Domain 계층 API

### Primitives

#### Expected<T>

**네임스페이스**: `winsetup::domain`

**설명**: Monadic 에러 처리를 위한 타입. 성공값 또는 에러를 담을 수 있습니다.

**생성자**:
```cpp
// 성공값으로 생성
Expected(T value);

// 에러로 생성
Expected(Error error);
```

**주요 메서드**:

##### HasValue
```cpp
[[nodiscard]] bool HasValue() const noexcept;
```
성공값을 포함하고 있는지 확인합니다.

**반환값**: 성공값이 있으면 `true`, 에러가 있으면 `false`

**예제**:
```cpp
auto result = GetDiskInfo(0);
if (result.HasValue()) {
    // 성공 처리
}
```

##### Value
```cpp
[[nodiscard]] T& Value() &;
[[nodiscard]] const T& Value() const &;
[[nodiscard]] T&& Value() &&;
```
성공값을 반환합니다.

**반환값**: 담고 있는 성공값의 참조

**주의**: 에러 상태에서 호출하면 assertion 실패

**예제**:
```cpp
auto result = GetDiskInfo(0);
if (result.HasValue()) {
    DiskInfo& disk = result.Value();
    std::wcout << disk.GetModelName() << std::endl;
}
```

##### GetError
```cpp
[[nodiscard]] const Error& GetError() const;
```
에러 정보를 반환합니다.

**반환값**: 담고 있는 에러 객체의 참조

**주의**: 성공 상태에서 호출하면 assertion 실패

**예제**:
```cpp
auto result = GetDiskInfo(0);
if (!result.HasValue()) {
    const Error& error = result.GetError();
    logger->Error(error.GetMessage());
}
```

##### Map
```cpp
template<typename F>
[[nodiscard]] auto Map(F&& func) -> Expected<U>;
```
성공값을 변환합니다. 에러는 그대로 전파됩니다.

**파라미터**:
- `func`: 변환 함수 `U(const T&)`

**반환값**: 변환된 값을 담은 `Expected<U>` 또는 에러

**예제**:
```cpp
auto sizeResult = GetDiskInfo(0)
    .Map([](const DiskInfo& disk) {
        return disk.GetSize().GetBytes();
    });
```

##### FlatMap
```cpp
template<typename F>
[[nodiscard]] auto FlatMap(F&& func) -> Expected<U>;
```
성공값을 다른 `Expected`로 변환합니다.

**파라미터**:
- `func`: 변환 함수 `Expected<U>(const T&)`

**반환값**: 변환 결과 또는 에러

**예제**:
```cpp
auto layoutResult = GetDiskInfo(0)
    .FlatMap([&](const DiskInfo& disk) {
        return diskService->GetCurrentLayout(disk.GetIndex());
    });
```

##### UnwrapOr
```cpp
[[nodiscard]] T UnwrapOr(T defaultValue) &&;
```
성공값을 추출하거나, 에러일 경우 기본값을 반환합니다.

**파라미터**:
- `defaultValue`: 에러 시 반환할 기본값

**반환값**: 성공값 또는 기본값

**예제**:
```cpp
uint64_t size = GetDiskSize(0)
    .UnwrapOr(0);
```

##### OnError
```cpp
template<typename F>
Expected& OnError(F&& func) &;
```
에러 시 콜백을 실행합니다.

**파라미터**:
- `func`: 에러 처리 함수 `void(const Error&)`

**반환값**: 자기 자신 (체이닝 가능)

**예제**:
```cpp
auto result = GetDiskInfo(0)
    .OnError([&](const Error& err) {
        logger->Error(err.GetMessage());
    });
```

---

#### Error

**네임스페이스**: `winsetup::domain`

**설명**: 구조화된 에러 정보를 담는 클래스.

**생성자**:
```cpp
Error(
    std::wstring message,
    uint32_t code = 0,
    ErrorCategory category = ErrorCategory::Unknown
);
```

**파라미터**:
- `message`: 에러 메시지
- `code`: 에러 코드 (Win32 에러 코드 등)
- `category`: 에러 카테고리

**주요 메서드**:

##### GetMessage
```cpp
[[nodiscard]] const std::wstring& GetMessage() const noexcept;
```
에러 메시지를 반환합니다.

**예제**:
```cpp
Error error{L"Disk not found", ERROR_FILE_NOT_FOUND, ErrorCategory::Disk};
std::wcout << error.GetMessage() << std::endl;
```

##### GetCode
```cpp
[[nodiscard]] uint32_t GetCode() const noexcept;
```
에러 코드를 반환합니다.

##### GetCategory
```cpp
[[nodiscard]] ErrorCategory GetCategory() const noexcept;
```
에러 카테고리를 반환합니다.

##### ToString
```cpp
[[nodiscard]] std::wstring ToString() const;
```
컨텍스트를 포함한 전체 에러 정보를 문자열로 반환합니다.

**예제**:
```cpp
logger->Error(error.ToString());
```

---

#### ErrorCategory

**네임스페이스**: `winsetup::domain`

**설명**: 에러 카테고리 열거형.

```cpp
enum class ErrorCategory {
    Unknown,
    System,
    Disk,
    Volume,
    Imaging,
    Configuration,
    Network,
    Permission
};
```

**사용 예제**:
```cpp
return Error{
    L"Failed to open disk",
    GetLastError(),
    ErrorCategory::Disk
};
```

---

### Entities

#### DiskInfo

**네임스페이스**: `winsetup::domain`

**설명**: 물리 디스크 정보를 나타내는 엔티티.

**생성자**:
```cpp
DiskInfo(
    uint32_t index,
    uint64_t sizeBytes,
    BusType busType,
    std::wstring modelName,
    std::wstring serialNumber
);
```

**주요 메서드**:

##### GetIndex
```cpp
[[nodiscard]] uint32_t GetIndex() const noexcept;
```
디스크 인덱스를 반환합니다 (0부터 시작).

##### GetSize
```cpp
[[nodiscard]] const DiskSize& GetSize() const noexcept;
```
디스크 크기를 반환합니다.

##### GetBusType
```cpp
[[nodiscard]] BusType GetBusType() const noexcept;
```
버스 타입을 반환합니다.

##### GetModelName
```cpp
[[nodiscard]] const std::wstring& GetModelName() const noexcept;
```
디스크 모델명을 반환합니다.

##### IsRemovable
```cpp
[[nodiscard]] bool IsRemovable() const noexcept;
```
이동식 디스크인지 확인합니다.

##### HasPartitions
```cpp
[[nodiscard]] bool HasPartitions() const noexcept;
```
파티션이 있는지 확인합니다.

**예제**:
```cpp
DiskInfo disk{0, 500_GB, BusType::SATA, L"Samsung SSD", L"S12345"};

std::wcout << L"Disk " << disk.GetIndex() << L": "
           << disk.GetModelName() << L" ("
           << disk.GetSize().ToGB() << L" GB)" << std::endl;

if (disk.IsRemovable()) {
    std::wcout << L"  [Removable]" << std::endl;
}
```

---

#### VolumeInfo

**네임스페이스**: `winsetup::domain`

**설명**: 볼륨(파티션) 정보를 나타내는 엔티티.

**생성자**:
```cpp
VolumeInfo(
    std::wstring volumePath,
    std::optional<wchar_t> driveLetter,
    FileSystemType fileSystem,
    std::wstring label,
    uint64_t totalBytes,
    uint64_t freeBytes
);
```

**주요 메서드**:

##### GetVolumePath
```cpp
[[nodiscard]] const std::wstring& GetVolumePath() const noexcept;
```
볼륨 경로를 반환합니다 (예: `\\?\Volume{...}`).

##### GetDriveLetter
```cpp
[[nodiscard]] std::optional<wchar_t> GetDriveLetter() const noexcept;
```
드라이브 문자를 반환합니다 (있을 경우).

##### GetFileSystem
```cpp
[[nodiscard]] FileSystemType GetFileSystem() const noexcept;
```
파일 시스템 타입을 반환합니다.

##### GetLabel
```cpp
[[nodiscard]] const std::wstring& GetLabel() const noexcept;
```
볼륨 레이블을 반환합니다.

##### GetUsagePercentage
```cpp
[[nodiscard]] double GetUsagePercentage() const noexcept;
```
사용률을 백분율로 반환합니다.

**예제**:
```cpp
VolumeInfo volume{
    L"\\\\?\\Volume{...}",
    L'C',
    FileSystemType::NTFS,
    L"Windows",
    500_GB,
    200_GB
};

if (auto letter = volume.GetDriveLetter(); letter.has_value()) {
    std::wcout << L"Drive " << letter.value() << L": "
               << volume.GetLabel() << std::endl;
}

std::wcout << L"Usage: " << volume.GetUsagePercentage() 
           << L"%" << std::endl;
```

---

#### SetupConfig

**네임스페이스**: `winsetup::domain`

**설명**: 설치 구성 정보를 담는 엔티티.

**생성자**:
```cpp
SetupConfig();
```

**주요 메서드**:

##### GetImagePath
```cpp
[[nodiscard]] const std::wstring& GetImagePath() const noexcept;
```
WIM 이미지 경로를 반환합니다.

##### SetImagePath
```cpp
void SetImagePath(std::wstring path);
```
WIM 이미지 경로를 설정합니다.

##### GetImageIndex
```cpp
[[nodiscard]] uint32_t GetImageIndex() const noexcept;
```
WIM 이미지 인덱스를 반환합니다.

##### GetDriverPaths
```cpp
[[nodiscard]] const std::vector<std::wstring>& GetDriverPaths() const noexcept;
```
드라이버 경로 목록을 반환합니다.

##### IsCleanInstall
```cpp
[[nodiscard]] bool IsCleanInstall() const noexcept;
```
클린 설치 여부를 반환합니다.

##### ShouldFormatDisks
```cpp
[[nodiscard]] bool ShouldFormatDisks() const noexcept;
```
디스크 포맷 여부를 반환합니다.

**예제**:
```cpp
SetupConfig config;
config.SetImagePath(L"D:\\install.wim");
config.SetImageIndex(1);
config.AddDriverPath(L"D:\\drivers");
config.SetCleanInstall(true);

if (config.IsCleanInstall()) {
    std::wcout << L"Clean install mode" << std::endl;
}
```

---

### Value Objects

#### DiskSize

**네임스페이스**: `winsetup::domain`

**설명**: 디스크 크기를 나타내는 값 객체.

**생성자**:
```cpp
explicit DiskSize(uint64_t bytes);
```

**정적 팩토리 메서드**:
```cpp
static DiskSize FromBytes(uint64_t bytes);
static DiskSize FromKB(uint64_t kb);
static DiskSize FromMB(uint64_t mb);
static DiskSize FromGB(uint64_t gb);
static DiskSize FromTB(uint64_t tb);
```

**주요 메서드**:

##### GetBytes
```cpp
[[nodiscard]] uint64_t GetBytes() const noexcept;
```

##### ToKB / ToMB / ToGB / ToTB
```cpp
[[nodiscard]] double ToKB() const noexcept;
[[nodiscard]] double ToMB() const noexcept;
[[nodiscard]] double ToGB() const noexcept;
[[nodiscard]] double ToTB() const noexcept;
```

##### ToString
```cpp
[[nodiscard]] std::wstring ToString() const;
```
사람이 읽기 좋은 형식으로 반환 (예: "500 GB").

**예제**:
```cpp
DiskSize size = DiskSize::FromGB(500);
std::wcout << size.ToString() << std::endl;  // "500 GB"

if (size.GetBytes() >= DiskSize::FromGB(100).GetBytes()) {
    std::wcout << L"Large enough for installation" << std::endl;
}
```

---

#### FileSystemType

**네임스페이스**: `winsetup::domain`

**설명**: 파일 시스템 타입 열거형.

```cpp
enum class FileSystemType {
    Unknown,
    NTFS,
    FAT32,
    exFAT,
    ReFS
};
```

**사용 예제**:
```cpp
if (volume.GetFileSystem() == FileSystemType::NTFS) {
    std::wcout << L"NTFS file system detected" << std::endl;
}
```

---

#### BusType

**네임스페이스**: `winsetup::domain`

**설명**: 디스크 버스 타입 열거형.

```cpp
enum class BusType {
    Unknown,
    SCSI,
    ATAPI,
    ATA,
    IEEE1394,
    SSA,
    FibreChannel,
    USB,
    RAID,
    iSCSI,
    SAS,
    SATA,
    SD,
    MMC,
    Virtual,
    FileBackedVirtual,
    Spaces,
    NVMe
};
```

**사용 예제**:
```cpp
if (disk.GetBusType() == BusType::NVMe) {
    std::wcout << L"High-performance NVMe SSD" << std::endl;
}
```

---

### Services

#### DiskSortingService

**네임스페이스**: `winsetup::domain`

**설명**: 디스크 목록을 필터링하고 정렬하는 도메인 서비스.

**주요 메서드**:

##### FilterAndSort
```cpp
[[nodiscard]] static FilterAndSortResult FilterAndSort(
    const std::vector<DiskInfo>& disks
);
```
디스크 목록을 필터링하고 우선순위에 따라 정렬합니다.

**파라미터**:
- `disks`: 원본 디스크 목록

**반환값**: 필터링 및 정렬 결과

**필터링 규칙**:
1. 최소 크기: 64GB 이상
2. 이동식 디스크 제외
3. 시스템 디스크 제외 (현재 Windows가 설치된 디스크)

**정렬 우선순위**:
1. NVMe > SATA > 기타
2. 큰 용량 우선
3. 빈 디스크 우선

**예제**:
```cpp
auto disksResult = diskService->EnumerateDisks();
if (!disksResult.HasValue()) {
    return disksResult.GetError();
}

auto sortResult = DiskSortingService::FilterAndSort(disksResult.Value());

std::wcout << L"Total disks: " << sortResult.totalDisks << std::endl;
std::wcout << L"Installable disks: " 
           << sortResult.installableDisks.size() << std::endl;

for (const auto& disk : sortResult.installableDisks) {
    std::wcout << L"  - " << disk.GetModelName() << std::endl;
}
```

---

#### PartitionAnalyzer

**네임스페이스**: `winsetup::domain`

**설명**: 파티션 레이아웃을 분석하는 도메인 서비스.

**주요 메서드**:

##### Analyze
```cpp
[[nodiscard]] static Expected<PartitionAnalysisResult> Analyze(
    const std::vector<PartitionInfo>& partitions
);
```
파티션 목록을 분석하여 Windows 파티션 구조를 파악합니다.

**파라미터**:
- `partitions`: 분석할 파티션 목록

**반환값**: 분석 결과 또는 에러

**분석 항목**:
- EFI 시스템 파티션 존재 여부
- MSR (Microsoft Reserved) 파티션 존재 여부
- Windows 파티션 존재 여부
- 복구 파티션 존재 여부

**예제**:
```cpp
auto layoutResult = diskService->GetCurrentLayout(0);
if (layoutResult.HasValue()) {
    auto analysisResult = PartitionAnalyzer::Analyze(
        layoutResult.Value().partitions
    );
    
    if (analysisResult.HasValue()) {
        const auto& analysis = analysisResult.Value();
        
        if (analysis.hasEFIPartition) {
            std::wcout << L"UEFI system detected" << std::endl;
        }
        
        if (analysis.hasWindowsPartition) {
            std::wcout << L"Existing Windows installation found" 
                       << std::endl;
        }
    }
}
```

##### IsEFIPartition
```cpp
[[nodiscard]] static bool IsEFIPartition(const PartitionInfo& partition);
```
EFI 시스템 파티션인지 확인합니다.

**판별 기준**:
- GUID: `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`
- FAT32 파일 시스템
- 크기: 100MB ~ 500MB

##### IsWindowsPartition
```cpp
[[nodiscard]] static bool IsWindowsPartition(const PartitionInfo& partition);
```
Windows 설치 파티션인지 확인합니다.

**판별 기준**:
- NTFS 파일 시스템
- Windows 디렉터리 존재
- System32 디렉터리 존재

---

### Specifications

#### DiskSpecifications

**네임스페이스**: `winsetup::domain`

**설명**: 디스크 필터링을 위한 Specification 패턴 구현.

**주요 클래스**:

##### MinimumSizeSpecification
```cpp
class MinimumSizeSpecification : public ISpecification<DiskInfo> {
public:
    explicit MinimumSizeSpecification(uint64_t minBytes);
    
    [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override;
};
```
최소 크기 조건을 검사합니다.

**예제**:
```cpp
MinimumSizeSpecification minSize(DiskSize::FromGB(64).GetBytes());

for (const auto& disk : disks) {
    if (minSize.IsSatisfiedBy(disk)) {
        std::wcout << disk.GetModelName() << L" is large enough" 
                   << std::endl;
    }
}
```

##### IsRemovableSpecification
```cpp
class IsRemovableSpecification : public ISpecification<DiskInfo> {
public:
    [[nodiscard]] bool IsSatisfiedBy(const DiskInfo& disk) const override;
};
```
이동식 디스크인지 검사합니다.

##### AndSpecification
```cpp
template<typename T>
class AndSpecification : public ISpecification<T> {
public:
    AndSpecification(
        std::unique_ptr<ISpecification<T>> left,
        std::unique_ptr<ISpecification<T>> right
    );
    
    [[nodiscard]] bool IsSatisfiedBy(const T& entity) const override;
};
```
두 Specification을 AND로 결합합니다.

**예제**:
```cpp
auto minSize = std::make_unique<MinimumSizeSpecification>(
    DiskSize::FromGB(100).GetBytes()
);
auto notRemovable = std::make_unique<NotSpecification<DiskInfo>>(
    std::make_unique<IsRemovableSpecification>()
);

auto combinedSpec = std::make_unique<AndSpecification<DiskInfo>>(
    std::move(minSize),
    std::move(notRemovable)
);

for (const auto& disk : disks) {
    if (combinedSpec->IsSatisfiedBy(disk)) {
        // 100GB 이상이고 고정식 디스크
    }
}
```

---

## Abstractions 계층 API

### Storage Services

#### IDiskService

**네임스페이스**: `winsetup::abstractions`

**설명**: 디스크 관리를 위한 인터페이스.

**주요 메서드**:

##### EnumerateDisks
```cpp
[[nodiscard]] virtual Expected<std::vector<DiskInfo>> EnumerateDisks() = 0;
```
시스템의 모든 물리 디스크를 열거합니다.

**반환값**: 디스크 목록 또는 에러

**예제**:
```cpp
auto result = diskService->EnumerateDisks();
if (result.HasValue()) {
    for (const auto& disk : result.Value()) {
        std::wcout << disk.GetModelName() << std::endl;
    }
}
```

##### GetDiskInfo
```cpp
[[nodiscard]] virtual Expected<DiskInfo> GetDiskInfo(uint32_t diskIndex) = 0;
```
특정 디스크의 정보를 가져옵니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스 (0부터 시작)

**반환값**: 디스크 정보 또는 에러

##### CleanDisk
```cpp
[[nodiscard]] virtual Expected<void> CleanDisk(uint32_t diskIndex) = 0;
```
디스크의 모든 파티션을 제거합니다.

**경고**: 데이터가 모두 삭제됩니다. 트랜잭션 내에서 사용 권장.

**파라미터**:
- `diskIndex`: 디스크 인덱스

**반환값**: 성공 또는 에러

##### CreatePartitionLayout
```cpp
[[nodiscard]] virtual Expected<void> CreatePartitionLayout(
    uint32_t diskIndex,
    const PartitionLayout& layout
) = 0;
```
파티션 레이아웃을 생성합니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스
- `layout`: 생성할 파티션 레이아웃

**반환값**: 성공 또는 에러

**예제**:
```cpp
PartitionLayout layout;
layout.style = PartitionLayout::Style::GPT;

// EFI 파티션
PartitionInfo efi;
efi.SetSize(DiskSize::FromMB(100));
efi.SetType(PartitionType::EFI);
efi.SetFileSystem(FileSystemType::FAT32);
layout.partitions.push_back(efi);

// Windows 파티션
PartitionInfo windows;
windows.SetSize(DiskSize::FromGB(200));
windows.SetType(PartitionType::Basic);
windows.SetFileSystem(FileSystemType::NTFS);
layout.partitions.push_back(windows);

auto result = diskService->CreatePartitionLayout(0, layout);
```

##### FormatPartition
```cpp
[[nodiscard]] virtual Expected<void> FormatPartition(
    uint32_t diskIndex,
    uint32_t partitionIndex,
    FileSystemType fileSystem,
    bool quickFormat = true
) = 0;
```
파티션을 포맷합니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스
- `partitionIndex`: 파티션 인덱스
- `fileSystem`: 파일 시스템 타입
- `quickFormat`: 빠른 포맷 여부

**반환값**: 성공 또는 에러

##### GetCurrentLayout
```cpp
[[nodiscard]] virtual Expected<PartitionLayout> GetCurrentLayout(
    uint32_t diskIndex
) = 0;
```
현재 파티션 레이아웃을 가져옵니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스

**반환값**: 파티션 레이아웃 또는 에러

##### RestoreLayout
```cpp
[[nodiscard]] virtual Expected<void> RestoreLayout(
    uint32_t diskIndex,
    const PartitionLayout& layout
) = 0;
```
파티션 레이아웃을 복원합니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스
- `layout`: 복원할 레이아웃

**반환값**: 성공 또는 에러

---

#### IVolumeService

**네임스페이스**: `winsetup::abstractions`

**설명**: 볼륨 관리를 위한 인터페이스.

**주요 메서드**:

##### EnumerateVolumes
```cpp
[[nodiscard]] virtual Expected<std::vector<VolumeInfo>> EnumerateVolumes() = 0;
```
시스템의 모든 볼륨을 열거합니다.

##### GetVolumeInfo
```cpp
[[nodiscard]] virtual Expected<VolumeInfo> GetVolumeInfo(
    const std::wstring& volumePath
) = 0;
```
특정 볼륨의 정보를 가져옵니다.

##### MountVolume
```cpp
[[nodiscard]] virtual Expected<void> MountVolume(
    const std::wstring& volumePath,
    wchar_t driveLetter
) = 0;
```
볼륨을 지정된 드라이브 문자에 마운트합니다.

**파라미터**:
- `volumePath`: 볼륨 경로 (예: `\\?\Volume{...}`)
- `driveLetter`: 드라이브 문자 (예: `'C'`)

**예제**:
```cpp
auto result = volumeService->MountVolume(
    L"\\\\?\\Volume{12345678-1234-1234-1234-123456789012}",
    L'X'
);

if (result.HasValue()) {
    std::wcout << L"Mounted as X:" << std::endl;
}
```

##### DismountVolume
```cpp
[[nodiscard]] virtual Expected<void> DismountVolume(wchar_t driveLetter) = 0;
```
드라이브 문자에서 볼륨을 마운트 해제합니다.

---

#### IImagingService

**네임스페이스**: `winsetup::abstractions`

**설명**: WIM 이미지 처리를 위한 인터페이스.

**주요 메서드**:

##### ApplyImage
```cpp
[[nodiscard]] virtual Expected<void> ApplyImage(
    const std::wstring& wimPath,
    uint32_t imageIndex,
    const std::wstring& targetPath,
    ProgressCallback progressCallback = nullptr
) = 0;
```
WIM 이미지를 대상 경로에 적용합니다.

**파라미터**:
- `wimPath`: WIM 파일 경로
- `imageIndex`: 이미지 인덱스 (1부터 시작)
- `targetPath`: 대상 경로 (예: `C:\`)
- `progressCallback`: 진행률 콜백 (선택)

**반환값**: 성공 또는 에러

**예제**:
```cpp
auto progressCallback = [](const ImageProgress& progress) {
    std::wcout << L"Progress: " << progress.percentComplete 
               << L"% (" << progress.currentFile << L")" << std::endl;
};

auto result = imagingService->ApplyImage(
    L"D:\\install.wim",
    1,
    L"C:\\",
    progressCallback
);
```

##### GetImageInfo
```cpp
[[nodiscard]] virtual Expected<std::vector<ImageInfo>> GetImageInfo(
    const std::wstring& wimPath
) = 0;
```
WIM 파일에 포함된 이미지 목록을 가져옵니다.

**파라미터**:
- `wimPath`: WIM 파일 경로

**반환값**: 이미지 정보 목록 또는 에러

**예제**:
```cpp
auto result = imagingService->GetImageInfo(L"D:\\install.wim");
if (result.HasValue()) {
    for (const auto& info : result.Value()) {
        std::wcout << L"[" << info.imageIndex << L"] "
                   << info.name << L" - "
                   << info.description << std::endl;
    }
}
```

##### SetCompressionLevel
```cpp
virtual void SetCompressionLevel(uint32_t level) = 0;
```
압축 레벨을 설정합니다 (1: 최소 ~ 9: 최대).

##### SetThreadCount
```cpp
virtual void SetThreadCount(uint32_t threads) = 0;
```
멀티스레드 처리에 사용할 스레드 개수를 설정합니다.

---

### Infrastructure Services

#### ILogger

**네임스페이스**: `winsetup::abstractions`

**설명**: 로깅을 위한 인터페이스.

**주요 메서드**:

##### Log
```cpp
virtual void Log(
    LogLevel level,
    const std::wstring& message,
    const std::source_location& location = std::source_location::current()
) = 0;
```
지정된 레벨로 로그를 기록합니다.

##### Trace / Debug / Info / Warning / Error / Fatal
```cpp
void Trace(const std::wstring& message);
void Debug(const std::wstring& message);
void Info(const std::wstring& message);
void Warning(const std::wstring& message);
void Error(const std::wstring& message);
void Fatal(const std::wstring& message);
```
각 레벨별 로그를 기록하는 편의 메서드.

**예제**:
```cpp
logger->Info(L"Starting disk enumeration");
logger->Debug(L"Opening disk 0");

auto result = diskService->GetDiskInfo(0);
if (!result.HasValue()) {
    logger->Error(L"Failed to get disk info: " + 
                  result.GetError().GetMessage());
}
```

##### Flush
```cpp
virtual void Flush() = 0;
```
버퍼에 있는 로그를 즉시 기록합니다.

**예제**:
```cpp
logger->Fatal(L"Critical error occurred");
logger->Flush();  // 즉시 파일에 기록
```

---

#### IEventBus

**네임스페이스**: `winsetup::abstractions`

**설명**: 이벤트 기반 통신을 위한 인터페이스.

**주요 메서드**:

##### Publish
```cpp
[[nodiscard]] virtual Expected<void> Publish(
    std::unique_ptr<IEvent> event,
    EventPriority priority = EventPriority::Normal
) = 0;

template<typename TEvent>
[[nodiscard]] Expected<void> Publish(
    TEvent event,
    EventPriority priority = EventPriority::Normal
);
```
이벤트를 발행합니다.

**파라미터**:
- `event`: 발행할 이벤트
- `priority`: 이벤트 우선순위

**반환값**: 성공 또는 에러

**예제**:
```cpp
InstallProgressEvent event;
event.stage = L"Applying image";
event.percentage = 50;
event.currentFile = L"windows\\system32\\ntoskrnl.exe";

auto result = eventBus->Publish(std::move(event));
```

##### Subscribe
```cpp
[[nodiscard]] virtual Expected<SubscriptionToken> Subscribe(
    std::type_index eventType,
    EventHandler handler
) = 0;

template<typename TEvent>
[[nodiscard]] Expected<SubscriptionToken> Subscribe(
    std::function<void(const TEvent&)> handler
);
```
이벤트를 구독합니다.

**파라미터**:
- `handler`: 이벤트 핸들러 함수

**반환값**: 구독 토큰 또는 에러

**예제**:
```cpp
auto token = eventBus->Subscribe<InstallProgressEvent>(
    [](const InstallProgressEvent& event) {
        std::wcout << L"Progress: " << event.percentage << L"%" 
                   << std::endl;
    }
);
```

##### Unsubscribe
```cpp
[[nodiscard]] virtual Expected<void> Unsubscribe(
    SubscriptionToken token
) = 0;
```
이벤트 구독을 취소합니다.

**파라미터**:
- `token`: 구독 시 받은 토큰

---

#### ITransaction

**네임스페이스**: `winsetup::abstractions`

**설명**: 트랜잭션 관리를 위한 인터페이스.

**주요 메서드**:

##### Begin
```cpp
[[nodiscard]] virtual Expected<void> Begin() = 0;
```
트랜잭션을 시작합니다.

##### Commit
```cpp
[[nodiscard]] virtual Expected<void> Commit() = 0;
```
트랜잭션을 커밋합니다.

##### Rollback
```cpp
[[nodiscard]] virtual Expected<void> Rollback() = 0;
```
트랜잭션을 롤백합니다.

##### Execute
```cpp
[[nodiscard]] virtual Expected<void> Execute(
    std::function<Expected<void>()> operation
) = 0;
```
트랜잭션 내에서 작업을 실행합니다.

**파라미터**:
- `operation`: 실행할 작업

**반환값**: 성공 또는 에러

**예제**:
```cpp
auto transaction = transactionManager->CreateDiskTransaction(0);

auto result = transaction->Execute([&]() -> Expected<void> {
    auto cleanResult = diskService->CleanDisk(0);
    if (!cleanResult.HasValue()) return cleanResult;
    
    auto layoutResult = diskService->CreatePartitionLayout(0, layout);
    if (!layoutResult.HasValue()) return layoutResult;
    
    return Expected<void>();
});

if (!result.HasValue()) {
    // 자동으로 롤백됨
}
```

---

## Application 계층 API

### Core

#### DIContainer

**네임스페이스**: `winsetup::application`

**설명**: 의존성 주입 컨테이너.

**주요 메서드**:

##### Register
```cpp
template<typename TInterface, typename TImplementation>
void Register(ServiceLifetime lifetime = ServiceLifetime::Singleton);
```
서비스를 등록합니다.

**파라미터**:
- `TInterface`: 인터페이스 타입
- `TImplementation`: 구현 타입
- `lifetime`: 서비스 수명

**예제**:
```cpp
DIContainer container;

container.Register<ILogger, Win32Logger>(ServiceLifetime::Singleton);
container.Register<IDiskService, Win32DiskService>(ServiceLifetime::Singleton);
```

##### RegisterWithDependencies
```cpp
template<typename TInterface, typename TImplementation, typename... TDeps>
void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton);
```
의존성이 있는 서비스를 등록합니다.

**파라미터**:
- `TDeps...`: 의존성 타입들

**예제**:
```cpp
container.RegisterWithDependencies<
    InstallWindowsUseCase,
    InstallWindowsUseCase,
    IDiskService,
    IImagingService,
    ILogger,
    IEventBus
>(ServiceLifetime::Transient);
```

##### RegisterInstance
```cpp
template<typename TInterface>
void RegisterInstance(std::shared_ptr<TInterface> instance);
```
이미 생성된 인스턴스를 등록합니다.

**예제**:
```cpp
auto logger = std::make_shared<Win32Logger>(L"setup.log");
container.RegisterInstance<ILogger>(logger);
```

##### Resolve
```cpp
template<typename TInterface>
std::shared_ptr<TInterface> Resolve();
```
서비스를 해결(resolve)합니다.

**반환값**: 서비스 인스턴스 또는 `nullptr`

**예제**:
```cpp
auto diskService = container.Resolve<IDiskService>();
if (diskService) {
    auto result = diskService->EnumerateDisks();
}
```

---

### Use Cases

#### InstallWindowsUseCase

**네임스페이스**: `winsetup::application`

**설명**: Windows 설치 유즈케이스.

**생성자**:
```cpp
InstallWindowsUseCase(
    std::shared_ptr<IDiskService> diskService,
    std::shared_ptr<IImagingService> imagingService,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IEventBus> eventBus
);
```

**주요 메서드**:

##### Execute
```cpp
[[nodiscard]] Task<Expected<void>> Execute(
    const SetupConfig& config,
    const DiskInfo& targetDisk
);
```
Windows 설치를 실행합니다.

**파라미터**:
- `config`: 설치 구성
- `targetDisk`: 대상 디스크

**반환값**: 비동기 작업 (Task)

**예제**:
```cpp
auto useCase = container.Resolve<InstallWindowsUseCase>();

SetupConfig config;
config.SetImagePath(L"D:\\install.wim");
config.SetImageIndex(1);

DiskInfo targetDisk = /* ... */;

auto task = useCase->Execute(config, targetDisk);
while (!task.IsDone()) {
    task.Resume();
    // 다른 작업 수행
}

auto result = task.GetResult();
if (result.HasValue()) {
    logger->Info(L"Installation completed successfully");
} else {
    logger->Error(L"Installation failed: " + 
                  result.GetError().GetMessage());
}
```

---

#### AnalyzeDisksUseCase

**네임스페이스**: `winsetup::application`

**설명**: 디스크 분석 유즈케이스.

**생성자**:
```cpp
AnalyzeDisksUseCase(
    std::shared_ptr<IDiskService> diskService,
    std::shared_ptr<IVolumeService> volumeService,
    std::shared_ptr<ILogger> logger
);
```

**주요 메서드**:

##### Execute
```cpp
[[nodiscard]] Expected<DiskAnalysisResult> Execute();
```
모든 디스크를 분석하여 설치 가능 여부를 판단합니다.

**반환값**: 분석 결과 또는 에러

**예제**:
```cpp
auto useCase = container.Resolve<AnalyzeDisksUseCase>();
auto result = useCase->Execute();

if (result.HasValue()) {
    const auto& analysis = result.Value();
    
    std::wcout << L"Total disks: " << analysis.totalDisks << std::endl;
    std::wcout << L"Installable disks: " 
               << analysis.installableDisks.size() << std::endl;
    
    for (const auto& disk : analysis.installableDisks) {
        std::wcout << L"  - " << disk.GetModelName() 
                   << L" (" << disk.GetSize().ToString() << L")" 
                   << std::endl;
    }
}
```

---

### Async

#### Task<T>

**네임스페이스**: `winsetup::application`

**설명**: C++20 코루틴을 위한 Task 타입.

**주요 메서드**:

##### Resume
```cpp
void Resume();
```
코루틴을 재개합니다.

##### IsDone
```cpp
bool IsDone() const;
```
작업이 완료되었는지 확인합니다.

##### GetResult
```cpp
Expected<T> GetResult();
```
작업 결과를 가져옵니다.

**주의**: `IsDone()`이 `true`일 때만 호출해야 합니다.

**예제**:
```cpp
Task<Expected<void>> AsyncOperation() {
    // 비동기 작업
    co_return Expected<void>();
}

auto task = AsyncOperation();
while (!task.IsDone()) {
    task.Resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

auto result = task.GetResult();
```

---

## Adapters 계층 API

### Win32 Platform

#### UniqueHandle

**네임스페이스**: `winsetup::adapters::win32`

**설명**: HANDLE의 RAII 래퍼.

**생성자**:
```cpp
explicit UniqueHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept;
```

**주요 메서드**:

##### Get
```cpp
[[nodiscard]] HANDLE Get() const noexcept;
```
핸들을 가져옵니다.

##### Release
```cpp
[[nodiscard]] HANDLE Release() noexcept;
```
소유권을 포기하고 핸들을 반환합니다.

##### Reset
```cpp
void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept;
```
새 핸들로 교체합니다.

**예제**:
```cpp
UniqueHandle hDisk(CreateFile(
    L"\\\\.\\PhysicalDrive0",
    GENERIC_READ,
    FILE_SHARE_READ,
    nullptr,
    OPEN_EXISTING,
    0,
    nullptr
));

if (hDisk) {
    // 핸들 사용
    DeviceIoControl(hDisk.Get(), ...);
}
// 자동으로 CloseHandle 호출됨
```

---

#### Win32HandleFactory

**네임스페이스**: `winsetup::adapters::win32`

**설명**: Win32 핸들 생성 팩토리.

**주요 메서드**:

##### OpenDisk
```cpp
[[nodiscard]] static Expected<UniqueHandle> OpenDisk(
    uint32_t diskIndex,
    DWORD accessFlags = GENERIC_READ | GENERIC_WRITE
);
```
디스크를 엽니다.

**파라미터**:
- `diskIndex`: 디스크 인덱스
- `accessFlags`: 접근 플래그

**반환값**: 핸들 또는 에러

**예제**:
```cpp
auto handleResult = Win32HandleFactory::OpenDisk(0);
if (handleResult.HasValue()) {
    UniqueHandle hDisk = std::move(handleResult.Value());
    // 핸들 사용
}
```

##### OpenVolume
```cpp
[[nodiscard]] static Expected<UniqueHandle> OpenVolume(
    const std::wstring& volumePath,
    DWORD accessFlags = GENERIC_READ
);
```
볼륨을 엽니다.

---

#### Win32StringHelper

**네임스페이스**: `winsetup::adapters::win32`

**설명**: 문자열 변환 및 조작 유틸리티.

**주요 메서드**:

##### UTF8ToWide
```cpp
[[nodiscard]] static Expected<std::wstring> UTF8ToWide(
    const std::string& utf8
);
```
UTF-8 문자열을 UTF-16으로 변환합니다.

##### WideToUTF8
```cpp
[[nodiscard]] static Expected<std::string> WideToUTF8(
    const std::wstring& wide
);
```
UTF-16 문자열을 UTF-8로 변환합니다.

##### FormatErrorMessage
```cpp
[[nodiscard]] static std::wstring FormatErrorMessage(DWORD errorCode);
```
Win32 에러 코드를 메시지로 변환합니다.

**예제**:
```cpp
DWORD error = GetLastError();
std::wstring message = Win32StringHelper::FormatErrorMessage(error);
logger->Error(message);
```

---

### Storage

#### DiskTransaction

**네임스페이스**: `winsetup::adapters::win32`

**설명**: Step 기반 디스크 트랜잭션.

**생성자**:
```cpp
DiskTransaction(
    uint32_t diskIndex,
    std::shared_ptr<IDiskService> diskService,
    std::shared_ptr<ILogger> logger
);
```

**주요 메서드**:

##### Execute
```cpp
[[nodiscard]] Expected<void> Execute(
    std::function<Expected<void>()> operation
);
```
트랜잭션을 실행합니다.

##### AddCleanDiskStep
```cpp
void AddCleanDiskStep();
```
디스크 정리 단계를 추가합니다.

##### AddCreatePartitionLayoutStep
```cpp
void AddCreatePartitionLayoutStep(const PartitionLayout& layout);
```
파티션 레이아웃 생성 단계를 추가합니다.

##### AddFormatPartitionStep
```cpp
void AddFormatPartitionStep(
    uint32_t partitionIndex,
    FileSystemType fileSystem,
    bool quickFormat = true
);
```
파티션 포맷 단계를 추가합니다.

##### ExecuteSteps
```cpp
[[nodiscard]] Expected<void> ExecuteSteps();
```
추가된 모든 단계를 실행합니다.

**예제**:
```cpp
DiskTransaction transaction(0, diskService, logger);

transaction.AddCleanDiskStep();

PartitionLayout layout;
// layout 구성...
transaction.AddCreatePartitionLayoutStep(layout);

transaction.AddFormatPartitionStep(0, FileSystemType::NTFS);

auto result = transaction.Execute([&]() {
    return transaction.ExecuteSteps();
});

if (!result.HasValue()) {
    // 자동 롤백됨
    logger->Error(L"Transaction failed: " + 
                  result.GetError().GetMessage());
}
```

---

#### MFTScanner

**네임스페이스**: `winsetup::adapters::win32`

**설명**: NTFS MFT를 직접 읽어 고속 파일 스캔.

**주요 메서드**:

##### ScanVolume
```cpp
[[nodiscard]] static Expected<VolumeContents> ScanVolume(
    const std::wstring& volumePath
);
```
볼륨의 내용을 스캔합니다.

**파라미터**:
- `volumePath`: 볼륨 경로

**반환값**: 볼륨 내용 분석 결과 또는 에러

**예제**:
```cpp
auto result = MFTScanner::ScanVolume(L"C:");
if (result.HasValue()) {
    const auto& contents = result.Value();
    
    if (contents.hasWindowsDirectory) {
        std::wcout << L"Windows installation detected" << std::endl;
    }
    
    std::wcout << L"Total files: " << contents.totalFiles << std::endl;
    std::wcout << L"Total directories: " << contents.totalDirectories 
               << std::endl;
}
```

---

### Imaging

#### WimlibAdapter

**네임스페이스**: `winsetup::adapters`

**설명**: wimlib 라이브러리 어댑터.

**주요 메서드**는 `IImagingService` 인터페이스와 동일하므로 위의 [IImagingService](#iimagingservice) 섹션을 참고하세요.

**추가 메서드**:

##### OptimizeImage
```cpp
[[nodiscard]] Expected<void> OptimizeImage(
    const std::wstring& wimPath,
    CompressionType compression = CompressionType::LZX
);
```
WIM 이미지를 최적화하고 재압축합니다.

**예제**:
```cpp
auto result = wimlibAdapter->OptimizeImage(
    L"D:\\install.wim",
    CompressionType::LZMS
);

if (result.HasValue()) {
    logger->Info(L"Image optimized successfully");
}
```

---

## 📊 예제 시나리오

### 완전한 Windows 설치 플로우

```cpp
#include "main/ServiceRegistration.h"

int wmain() {
    // 1. 서비스 등록
    DIContainer container;
    RegisterAllServices(container);
    
    // 2. 로거 초기화
    auto logger = container.Resolve<ILogger>();
    logger->Info(L"Starting WinSetup");
    
    // 3. 구성 로드
    auto configRepo = container.Resolve<IConfigRepository>();
    auto configResult = configRepo->Load(L"config.ini");
    if (!configResult.HasValue()) {
        logger->Fatal(L"Failed to load configuration");
        return 1;
    }
    const auto& config = configResult.Value();
    
    // 4. 디스크 분석
    auto analyzeUseCase = container.Resolve<AnalyzeDisksUseCase>();
    auto analysisResult = analyzeUseCase->Execute();
    if (!analysisResult.HasValue()) {
        logger->Fatal(L"Disk analysis failed");
        return 1;
    }
    
    const auto& analysis = analysisResult.Value();
    if (analysis.installableDisks.empty()) {
        logger->Fatal(L"No suitable disk found");
        return 1;
    }
    
    // 5. 사용자에게 디스크 선택 UI 표시
    const auto& targetDisk = analysis.installableDisks;
    logger->Info(L"Selected disk: " + targetDisk.GetModelName());
    
    // 6. Windows 설치 실행
    auto installUseCase = container.Resolve<InstallWindowsUseCase>();
    auto installTask = installUseCase->Execute(config, targetDisk);
    
    while (!installTask.IsDone()) {
        installTask.Resume();
        // UI 업데이트 등
    }
    
    auto installResult = installTask.GetResult();
    if (!installResult.HasValue()) {
        logger->Fatal(L"Installation failed: " + 
                      installResult.GetError().GetMessage());
        return 1;
    }
    
    logger->Info(L"Installation completed successfully");
    return 0;
}
```

---

**문서 버전**: 1.0  
**최종 업데이트**: 2026년 2월 15일  
**작성자**: 한준희
