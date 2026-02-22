# WinSetup 클린 아키텍처 구현 계획서 v1.0

## 🎯 개요

### 목표
Windows PE 환경에서 실행되는 완벽한 PC 초기화 프로그램을 **클린 아키텍처 + 저수준 최적화 + 프로덕션 안정성**을 모두 충족하여 구현합니다.

### 핵심 설계 원칙
1. **완전한 계층 격리**: Domain은 외부 의존성 0 (Windows.h 절대 금지)
2. **타입 안전성**: void* 금지, 컴파일 타임 타입 체크
3. **RAII 강제**: 모든 리소스는 자동 정리
4. **트랜잭션**: 원자성 보장, 실패 시 롤백
5. **고성능**: IOCTL, MFT, wimlib 최적화
6. **테스트 가능성**: 모든 비즈니스 로직 단위 테스트
7. **MVVM 패턴**: UI와 비즈니스 로직의 완전한 분리

### 기술 스택
- **언어**: C++20/23 (Concepts, Coroutines, Ranges)
- **플랫폼**: Windows PE (Windows 10/11)
- **빌드**: Visual Studio 2022 Solution (.sln + .vcxproj)
- **UI 패턴**: MVVM (Model-View-ViewModel)
- **저수준 API**:
  - IOCTL (디스크 제어)
  - FSCTL (MFT 직접 읽기)
  - GetSystemFirmwareTable (SMBIOS)
  - OVERLAPPED (진정한 비동기 I/O)
  - wimlib (이미지 처리)
  - DismApi (드라이버 주입)
  - FveAPI (BitLocker 프로그래밍 방식 활성화)
- **테스트**: Google Test + Google Mock

### 예상 성능 지표
- **디스크 열거**: 10개 디스크 < 3초 (병렬 IOCTL)
- **파티션 분석**: 10개 볼륨 < 1초 (MFT 직접 읽기)
- **이미지 적용**: Config.ini 예상 시간 준수
- **메모리 사용**: WinPE 환경 < 512MB
- **Config.ini 로드**: < 100ms

---

## 📐 아키텍처 원칙

### 1. 절대 의존성 규칙 (Absolute Dependency Rule)

```
┌─────────────────────────────────────────┐
│  Domain (Layer 1)                       │
│  ✓ 외부 의존성 0                         │
│  ✓ 표준 C++만 사용                       │
│  ✓ Windows.h 절대 금지                  │
│  ✓ Expected, Error 등 기본 제공          │
│  ✓ IEvent / DomainEventBase 정의        │
│  ✗ HANDLE, HWND 등 플랫폼 타입 금지      │
└─────────────────────────────────────────┘
            ↑
            │ (Domain 타입만 참조)
┌─────────────────────────────────────────┐
│  Abstractions (Layer 0)                 │
│  ✓ 순수 인터페이스                       │
│  ✓ Domain 타입 사용                      │
│  ✓ IEvent = domain::DomainEvent (alias) │
│  ✓ SystemAnalysisResult 정의            │
│  ✓ IBitLockerService 정의               │
│  ✗ 구현 코드 금지                        │
└─────────────────────────────────────────┘
            ↑
            │ (인터페이스만 의존)
┌─────────────────────────────────────────┐
│  Application (Layer 2)                  │
│  ✓ Use Cases                            │
│  ✓ ViewModels (MVVM)                    │
│  ✓ Task<T> 코루틴                       │
│  ✓ DIContainer                          │
│  ✓ ConfigureBitLockerUseCase            │
│  ✗ 플랫폼 코드 금지                      │
└─────────────────────────────────────────┘
            ↑
            │ (인터페이스 구현)
┌─────────────────────────────────────────┐
│  Adapters (Layer 3)                     │
│  ✓ IOCTL + OVERLAPPED                   │
│  ✓ MFT 직접 읽기                         │
│  ✓ SMBIOS 파싱                          │
│  ✓ wimlib 최적화                        │
│  ✓ DiskTransaction                      │
│  ✓ Win32 RAII 래퍼 (UniqueHandle 등)    │
│  ✓ INI 파일 파서                         │
│  ✓ Win32 View (MVVM)                    │
│  ✓ FveApiBitLockerService               │
└─────────────────────────────────────────┘
            ↑
            │ (모든 것 연결)
┌─────────────────────────────────────────┐
│  Main (Layer 4)                         │
│  ✓ Composition Root                     │
│  ✓ Service Registration                 │
│  ✓ Resolve 실패 시 즉시 throw           │
└─────────────────────────────────────────┘
```

### 2. 레이어 간 의존성 핵심 결정 사항

| 결정 사항 | 내용 |
|---|---|
| `IEvent` 위치 | `domain::DomainEvent`로 정의, `abstractions`는 `using` 별칭 |
| `SystemAnalysisResult` 위치 | `abstractions::SystemAnalysisResult`로 정의, `application`은 `using` 별칭 |
| `DIContainer` 저장 타입 | `shared_ptr<void>` 유지 (소멸자 안전), 등록 시 인터페이스 타입으로 캐스트 강제 |
| Composition Root 실패 처리 | `ResolveOrThrow` 헬퍼로 즉시 `std::runtime_error` throw |
| GDI 리소스 캐싱 | `OnPaint`마다 `CreateFont` 금지, `Create()` 시점에 멤버 `UniqueHandle`로 캐싱 |
| 멤버 변수 네이밍 | 전체 `mXxx` camelCase 통일 (예외 없음) |
| BitLocker 활성화 조건 | `config.ini` `[BITLOCKER]` 섹션 `ENABLED=Yes` 시에만 실행 |
| BitLocker PIN | `config.ini` `[BITLOCKER]` 섹션 `PINNUMBER` 값 사용 |
| BitLocker Data 자동 해제 | Windows AutoUnlock 기능 사용 (`manage-bde -autounlock -enable D:` 동작) |
| 복구 정보 저장 | `<exe폴더>/bitlocker/bitlocker.txt` — `UID : 복구비밀번호` 형식 |
| 복구 키 파일 | Boot 파티션 루트에 `<UID>.key` 빈 파일 생성 |

### 3. MVVM 패턴 적용

```
┌─────────────────────────────────────────┐
│  View (Win32MainWindow)                 │
│  - 순수 UI 렌더링                        │
│  - 사용자 입력 수신                      │
│  - ViewModel에게 명령 전달               │
│  - ViewModel 속성 변경 감지              │
└─────────────────────────────────────────┘
            ↓ IPropertyChanged
┌─────────────────────────────────────────┐
│  ViewModel (MainViewModel)              │
│  - 프레젠테이션 로직                     │
│  - 상태 관리 (StatusText, Progress)     │
│  - UseCase 호출                         │
│  - SystemAnalysisResult 단일 소유       │
└─────────────────────────────────────────┘
            ↓ Domain/Application
┌─────────────────────────────────────────┐
│  Model (Entities, UseCases)             │
│  - 비즈니스 로직                         │
│  - Domain Entities                      │
│  - Services                             │
└─────────────────────────────────────────┘
```

**핵심 원칙**:
- View는 ViewModel의 인터페이스만 알고 있음
- ViewModel은 View를 전혀 모름 (IPropertyChanged 콜백만 사용)
- View는 HWND, HDC 등 Win32 타입만 다룸
- ViewModel은 std::wstring, bool, int 등 표준 타입만 다룸

---

## 🔐 BitLocker 설계

### 개요

설치 완료 후 `config.ini`의 `[BITLOCKER]` 섹션 `ENABLED=Yes` 설정이 활성화된 경우에만 실행됩니다.
FveAPI(`fveapi.dll`)를 동적 로딩하여 아래 `manage-bde` 명령어와 동일한 동작을 수행합니다.

```
manage-bde -on C: -skiphardwaretest
manage-bde -protectors -add C: -TPMAndPIN <PINNUMBER>
manage-bde -on D: -RecoveryPassword
manage-bde -autounlock -enable D:
```

### config.ini 설정

```ini
[BITLOCKER]
ENABLED=Yes
PINNUMBER=900000
```

| 키 | 설명 | 기본값 |
|---|---|---|
| `ENABLED` | `Yes` 일 때만 BitLocker 활성화 실행 | `No` |
| `PINNUMBER` | TPM+PIN 프로텍터에 사용할 숫자 PIN | 없음 |

### 활성화 흐름

```
InstallWindowsUseCase 완료
        ↓
ConfigureBitLockerUseCase::Execute()
        ↓  config.ini ENABLED=Yes 확인
        ↓  config.ini PINNUMBER 읽기
        │
        ├──  System 파티션 (C:) 암호화[1]
        │     ├── FVE_KEY_PROTECTOR_TYPE = FVE_KEY_PROTECTOR_TPMANDPIN
        │     ├── PINNUMBER으로 TPM+PIN 프로텍터 추가
        │     ├── 48자리 복구 비밀번호 프로텍터 추가
        │     └── 암호화 시작 (SkipHardwareTest)
        │
        ├──  Data 파티션 (D:) 암호화[2]
        │     ├── 복구 비밀번호 프로텍터 추가
        │     ├── 암호화 시작
        │     └── Windows AutoUnlock 활성화
        │           → System 파티션(C:) 잠금 해제 시
        │              Data 파티션(D:) 자동 잠금 해제
        │
        └──  복구 정보 저장
              ├── <exe폴더>/bitlocker/bitlocker.txt 기록
              └── Boot 파티션 루트에 <UID>.key 빈 파일 생성
```

### Windows AutoUnlock 동작 원리

`manage-bde -autounlock -enable D:`는 Windows가 System 파티션(C:)의 잠금이 해제될 때 Data 파티션(D:)을 자동으로 잠금 해제하도록 등록합니다. AutoUnlock 외부 키는 System 파티션의 메타데이터 영역에 저장되며, Windows 부팅 시 TPM+PIN으로 C:가 해제되면 D:는 별도 PIN 입력 없이 자동 마운트됩니다.

FveAPI 구현 대응:

| manage-bde 명령 | FveAPI 함수 |
|---|---|
| `-on C: -skiphardwaretest` | `FveVolEncrypt` + `FVE_ENCRYPT_FLAG_SKIP_HARDWARE_TEST` |
| `-protectors -add C: -TPMAndPIN` | `FveKeyProtectorAddTpmAndPin` |
| `-protectors -add -RecoveryPassword` | `FveKeyProtectorAddNumericalPassword` |
| `-autounlock -enable D:` | `FveVolumeEnableAutoUnlock` |

### 복구 정보 저장 구조

```
<실행파일 폴더>/
└── bitlocker/
    └── bitlocker.txt
```

`bitlocker.txt` 형식:
```
[System]
UID : XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX : XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX

[Data]
UID : XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX : XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX
```

Boot 파티션 루트에는 각 볼륨 UID 기반 빈 키 파일을 생성합니다:
```
<Boot 파티션 루트>/
├── XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX.key   ← System 파티션
└── XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX.key   ← Data 파티션
```

### 계층별 책임

| 계층 | 파일 | 책임 |
|---|---|---|
| Abstractions | `IBitLockerService.h` | 인터페이스 정의 |
| Abstractions | `IConfigureBitLockerUseCase.h` | UseCase 인터페이스 |
| Application | `ConfigureBitLockerUseCase.h/.cpp` | 비즈니스 로직, PIN 읽기, 복구 파일 저장 |
| Adapters | `FveApiBitLockerService.h/.cpp` | FveAPI 동적 로딩, Win32 구현 |

### IBitLockerService 인터페이스

```cpp
// abstractions/services/security/IBitLockerService.h
struct BitLockerVolumeInfo {
    std::wstring volumePath;
    std::wstring uid;
    std::wstring recoveryPassword;
};

class IBitLockerService {
public:
    virtual ~IBitLockerService() = default;
    virtual domain::Expected<BitLockerVolumeInfo> EncryptWithTpmAndPin(
        const std::wstring& volumePath,
        const std::wstring& pin) = 0;
    virtual domain::Expected<BitLockerVolumeInfo> EncryptWithRecoveryPassword(
        const std::wstring& volumePath) = 0;
    virtual domain::Expected<void> EnableAutoUnlock(
        const std::wstring& dataVolumePath) = 0;
    virtual domain::Expected<void> WriteKeyFile(
        const std::wstring& bootPartitionPath,
        const std::wstring& uid) = 0;
};
```

---

## 📂 폴더 구조

```
WinSetup/
│
├── WinSetup.sln
├── README.md
│
├── WinSetup/
│   ├── WinSetup.vcxproj
│   ├── WinSetup.vcxproj.filters
│   │
│   ├── src/
│   │   │
│   │   ├── abstractions/
│   │   │   ├── infrastructure/
│   │   │   │   ├── async/
│   │   │   │   │   ├── IAsyncContext.h
│   │   │   │   │   ├── IExecutor.h
│   │   │   │   │   ├── IScheduler.h
│   │   │   │   │   └── IThreadPool.h
│   │   │   │   ├── logging/
│   │   │   │   │   ├── ILogger.h
│   │   │   │   │   └── LogLevel.h
│   │   │   │   ├── messaging/
│   │   │   │   │   ├── IDispatcher.h
│   │   │   │   │   ├── IEvent.h              # using IEvent = domain::DomainEvent
│   │   │   │   │   ├── IEventBus.h
│   │   │   │   │   └── IMessageQueue.h
│   │   │   │   └── transaction/
│   │   │   │       ├── ITransaction.h
│   │   │   │       └── ITransactionManager.h
│   │   │   ├── repositories/
│   │   │   │   ├── IConfigRepository.h
│   │   │   │   ├── IDiskRepository.h
│   │   │   │   └── IVolumeRepository.h
│   │   │   ├── services/
│   │   │   │   ├── platform/
│   │   │   │   │   ├── ISystemInfoService.h
│   │   │   │   │   └── ITextEncoder.h
│   │   │   │   ├── security/
│   │   │   │   │   └── IBitLockerService.h   # BitLocker 인터페이스
│   │   │   │   └── storage/
│   │   │   │       ├── IDiskService.h
│   │   │   │       ├── IDriverService.h
│   │   │   │       ├── IFileCopyService.h
│   │   │   │       ├── IImagingService.h
│   │   │   │       ├── IPartitionService.h
│   │   │   │       ├── IStorageScanner.h
│   │   │   │       └── IVolumeService.h
│   │   │   ├── ui/
│   │   │   │   ├── IMainViewModel.h
│   │   │   │   ├── IProgressBar.h
│   │   │   │   ├── IPropertyChanged.h
│   │   │   │   ├── IWidget.h
│   │   │   │   └── IWindow.h
│   │   │   └── usecases/
│   │   │       ├── IAnalyzeSystemUseCase.h
│   │   │       ├── IConfigureBitLockerUseCase.h
│   │   │       └── ILoadConfigurationUseCase.h
│   │   │
│   │   ├── application/
│   │   │   ├── async/
│   │   │   │   ├── Awaitable.h
│   │   │   │   ├── CancellationToken.h / .cpp
│   │   │   │   ├── Promise.h
│   │   │   │   └── Task.h / .cpp
│   │   │   ├── core/
│   │   │   │   ├── DIContainer.h / .cpp
│   │   │   │   └── ServiceLocator.h / .cpp
│   │   │   ├── dto/
│   │   │   │   ├── DiskAnalysisResult.h
│   │   │   │   ├── InstallationProgress.h
│   │   │   │   └── SystemAnalysisResult.h
│   │   │   ├── eventhandlers/
│   │   │   │   ├── DiskAnalyzedEventHandler.h / .cpp
│   │   │   │   └── InstallProgressEventHandler.h / .cpp
│   │   │   ├── services/
│   │   │   │   ├── Dispatcher.h / .cpp
│   │   │   │   ├── EventBus.h / .cpp
│   │   │   │   └── TaskScheduler.h / .cpp
│   │   │   ├── usecases/
│   │   │   │   ├── disk/
│   │   │   │   │   ├── AnalyzeDisksUseCase.h / .cpp
│   │   │   │   │   ├── EnumerateDisksUseCase.h / .cpp
│   │   │   │   │   └── SelectTargetDisksUseCase.h / .cpp
│   │   │   │   ├── install/
│   │   │   │   │   ├── ApplyImageUseCase.h / .cpp
│   │   │   │   │   ├── BackupUserDataUseCase.h / .cpp
│   │   │   │   │   ├── InjectDriversUseCase.h / .cpp
│   │   │   │   │   ├── InstallWindowsUseCase.h / .cpp
│   │   │   │   │   └── RestoreUserDataUseCase.h / .cpp
│   │   │   │   └── system/
│   │   │   │       ├── AnalyzeSystemUseCase.h / .cpp
│   │   │   │       ├── ConfigureBitLockerUseCase.h / .cpp  # PIN은 SetupConfig에서 수신
│   │   │   │       └── LoadConfigurationUseCase.h / .cpp
│   │   │   └── viewmodels/
│   │   │       ├── MainViewModel.h
│   │   │       └── MainViewModel.cpp
│   │   │
│   │   ├── domain/
│   │   │   ├── entities/
│   │   │   │   ├── DiskInfo.h / .cpp
│   │   │   │   ├── PartitionInfo.h / .cpp
│   │   │   │   ├── SetupConfig.h / .cpp      # bitLockerEnabled, bitLockerPin 필드 추가
│   │   │   │   ├── SystemInfo.h
│   │   │   │   └── VolumeInfo.h / .cpp
│   │   │   ├── events/
│   │   │   │   ├── DomainEvent.h / .cpp
│   │   │   │   ├── DiskAnalyzedEvent.h
│   │   │   │   ├── ErrorOccurredEvent.h
│   │   │   │   ├── InstallCompletedEvent.h
│   │   │   │   ├── InstallProgressEvent.h
│   │   │   │   └── InstallStartedEvent.h
│   │   │   ├── functional/
│   │   │   │   ├── Compose.h
│   │   │   │   ├── Monads.h
│   │   │   │   ├── Optional.h
│   │   │   │   └── Pipeline.h
│   │   │   ├── memory/
│   │   │   │   └── PoolAllocator.h / .cpp
│   │   │   ├── primitives/
│   │   │   │   ├── Error.h / .cpp
│   │   │   │   ├── Expected.h / .cpp
│   │   │   │   └── Result.h
│   │   │   ├── services/
│   │   │   │   ├── DiskSortingService.h / .cpp
│   │   │   │   ├── PartitionAnalyzer.h / .cpp
│   │   │   │   └── PathNormalizer.h / .cpp
│   │   │   ├── specifications/
│   │   │   │   ├── DiskSpecifications.h / .cpp
│   │   │   │   ├── ISpecification.h
│   │   │   │   └── VolumeSpecifications.h / .cpp
│   │   │   └── valueobjects/
│   │   │       ├── BusType.h
│   │   │       ├── DiskSize.h / .cpp
│   │   │       ├── DiskType.h
│   │   │       ├── DriveLetter.h / .cpp
│   │   │       ├── FileSystemType.h
│   │   │       ├── InstallationType.h
│   │   │       └── PartitionType.h
│   │   │
│   │   ├── adapters/
│   │   │   ├── imaging/
│   │   │   │   ├── DismAdapter.h / .cpp
│   │   │   │   ├── WimlibAdapter.h / .cpp
│   │   │   │   └── WimlibOptimizer.h / .cpp
│   │   │   ├── persistence/
│   │   │   │   ├── config/
│   │   │   │   │   ├── IniConfigRepository.h / .cpp  # [BITLOCKER] 섹션 파싱 추가
│   │   │   │   │   └── IniParser.h / .cpp
│   │   │   │   └── filesystem/
│   │   │   │       └── Win32FileSystem.h / .cpp
│   │   │   ├── platform/win32/
│   │   │   │   ├── core/
│   │   │   │   │   ├── Win32Constants.h
│   │   │   │   │   ├── Win32ErrorHandler.h / .cpp
│   │   │   │   │   ├── Win32HandleFactory.h / .cpp
│   │   │   │   │   ├── Win32StringHelper.h / .cpp
│   │   │   │   │   └── Win32TypeMapper.h / .cpp
│   │   │   │   ├── logging/
│   │   │   │   │   └── Win32Logger.h / .cpp
│   │   │   │   ├── memory/
│   │   │   │   │   ├── UniqueFindHandle.h
│   │   │   │   │   ├── UniqueHandle.h / .cpp
│   │   │   │   │   └── UniqueLibrary.h
│   │   │   │   ├── security/
│   │   │   │   │   └── FveApiBitLockerService.h / .cpp  # FveAPI 구현체
│   │   │   │   ├── storage/
│   │   │   │   │   ├── AsyncIOCTL.h / .cpp
│   │   │   │   │   ├── DiskLayoutBuilder.h / .cpp
│   │   │   │   │   ├── DiskTransaction.h / .cpp
│   │   │   │   │   ├── IOCTLWrapper.h / .cpp
│   │   │   │   │   ├── MFTScanner.h / .cpp
│   │   │   │   │   ├── Win32DiskService.h / .cpp
│   │   │   │   │   ├── Win32FileCopyService.h / .cpp
│   │   │   │   │   └── Win32VolumeService.h / .cpp
│   │   │   │   ├── system/
│   │   │   │   │   ├── FirmwareTableReader.h
│   │   │   │   │   ├── SMBIOSParser.h / .cpp
│   │   │   │   │   ├── SMBIOSStructures.h
│   │   │   │   │   └── Win32SystemInfoService.h / .cpp
│   │   │   │   └── threading/
│   │   │   │       ├── Win32Thread.h / .cpp
│   │   │   │       └── Win32ThreadPool.h / .cpp
│   │   │   └── ui/win32/
│   │   │       ├── controls/
│   │   │       │   ├── SimpleButton.h / .cpp
│   │   │       │   ├── TextWidget.h / .cpp
│   │   │       │   ├── ToggleButton.h / .cpp
│   │   │       │   └── TypeSelectorGroup.h / .cpp
│   │   │       ├── panels/
│   │   │       │   ├── ActionPanel.h / .cpp
│   │   │       │   ├── OptionPanel.h / .cpp
│   │   │       │   └── StatusPanel.h / .cpp
│   │   │       ├── Win32MainWindow.h / .cpp
│   │   │       └── Win32ProgressBar.h / .cpp
│   │   │
│   │   └── main/
│   │       ├── Main.cpp
│   │       ├── ServiceRegistration.h
│   │       └── ServiceRegistration.cpp
│   │
│   ├── lib/
│   │   └── wimlib.h
│   │
│   └── config.ini
│
└── tests/
    ├── adapters/
    │   ├── DiskTransactionTests.cpp
    │   ├── MFTScannerTests.cpp
    │   ├── SMBIOSParserTests.cpp
    │   └── Win32DiskServiceTests.cpp
    ├── application/
    │   ├── core/
    │   │   └── DIContainerTests.cpp
    │   ├── services/
    │   │   └── EventBusTests.cpp
    │   └── usecases/
    │       ├── AnalyzeSystemUseCaseTests.cpp
    │       ├── ConfigureBitLockerUseCaseTests.cpp
    │       └── InstallWindowsUseCaseTests.cpp
    ├── domain/
    │   ├── entities/
    │   │   ├── DiskInfoTests.cpp
    │   │   └── SetupConfigTests.cpp
    │   ├── primitives/
    │   │   └── ExpectedTests.cpp
    │   └── services/
    │       ├── DiskSortingServiceTests.cpp
    │       ├── PartitionAnalyzerTests.cpp
    │       └── DiskSpecificationsTests.cpp
    ├── integration/
    │   ├── EndToEndTests.cpp
    │   └── PerformanceTests.cpp
    ├── mocks/
    │   ├── MockBitLockerService.h
    │   ├── MockDiskService.h
    │   ├── MockEventBus.h
    │   └── MockLogger.h
    └── TestMain.cpp
```

---

## 🔧 핵심 설계 결정 상세

### 1. 이벤트 시스템 단일화

`DomainEvent`를 Domain 계층에서 정의하고 `abstractions::IEvent`는 `using` 별칭으로 연결합니다.

```
domain::DomainEvent          ← 실제 정의
domain::DomainEventBase<T>   ← CRTP 헬퍼 (GetTypeIndex, GetEventType, Clone 자동 구현)
abstractions::IEvent         ← using IEvent = domain::DomainEvent
abstractions::EventBase<T>   ← using EventBase = domain::DomainEventBase<T>
```

도메인 이벤트 작성 규칙:
```cpp
class DiskAnalyzedEvent : public domain::DomainEventBase<DiskAnalyzedEvent> {
public:
    static std::wstring StaticEventType() noexcept { return L"DiskAnalyzed"; }
    std::wstring ToString() const override { ... }
};
```

### 2. SystemAnalysisResult 단일 진실 공급원

```
abstractions::SystemAnalysisResult  ← 실제 정의 (IAnalyzeSystemUseCase.h 내)
application::SystemAnalysisResult   ← using 별칭 (SystemAnalysisResult.h)
```

- `AnalyzeSystemUseCase::Execute()`에서 1회만 생성
- `MainViewModel`이 `shared_ptr<const abstractions::SystemAnalysisResult>`로 소유
- 모든 하위 UseCase는 `shared_ptr<const SystemAnalysisResult>`로 읽기 전용 수신

### 3. Composition Root 실패 처리

```cpp
template <typename TInterface>
std::shared_ptr<TInterface> ResolveOrThrow(DIContainer& container, const char* name) {
    auto result = container.Resolve<TInterface>();
    if (!result.HasValue())
        throw std::runtime_error(std::string("Critical: Failed to resolve ") + name);
    return result.Value();
}
```

`std::runtime_error`는 `Main.cpp`의 catch 블록에서 처리합니다.

### 4. GDI 리소스 캐싱 규칙

`OnPaint` 콜백 내 `CreateFont` 직접 호출 금지. `Create()` 시점에 `EnsureFonts()`를 호출하여 멤버 `UniqueHandle`로 1회 생성합니다.

```
모든 UI 컴포넌트의 폰트 관리 패턴 통일:
  Create() → EnsureFonts() → mFontXxx (UniqueHandle) 1회 생성
  OnPaint() → EnsureFonts() (guard) → SelectObject만 수행

적용 파일:
  StatusPanel       → mFontStatus, mFontDesc
  Win32ProgressBar  → mFontProgress, mFontTime
  TypeSelectorGroup → mLabelFont
  ToggleButton      → mhFont
  SimpleButton      → UniqueHandle
```

### 5. 멤버 변수 네이밍 규칙

프로젝트 전체 `mXxx` camelCase 단일 표준. `m_xxx`, `mxxx` 형태 금지.

### 6. BitLocker 구현 규칙

- PIN 번호는 `config.ini` `[BITLOCKER]` 섹션 `PINNUMBER` 값을 `SetupConfig`를 통해 수신
- `IniConfigRepository`가 `[BITLOCKER]` 섹션을 파싱하여 `SetupConfig::bitLockerEnabled`, `SetupConfig::bitLockerPin` 필드에 저장
- `FveApiBitLockerService`는 `fveapi.dll`을 `UniqueLibrary`로 동적 로딩
- `fveapi.dll` 로드 실패 시 `Expected<void>` 에러 반환, 프로세스 종료 없음
- System 파티션은 TPM+PIN 프로텍터 + 복구 비밀번호 프로텍터 추가 후 암호화
- Data 파티션은 복구 비밀번호 프로텍터 추가 후 암호화, Windows AutoUnlock 활성화
- AutoUnlock은 Windows가 System 파티션 해제 시 Data 파티션을 자동 해제하는 OS 내장 기능
- 복구 비밀번호 파일 저장은 `ConfigureBitLockerUseCase`가 담당 (플랫폼 코드 금지)
- Boot 파티션 경로는 `SystemAnalysisResult`에서 결정 — `IBitLockerService`는 경로를 수신할 뿐

---

## 📅 구현 단계

### ✅ 완료된 Phase

#### Phase 1: 기초 인프라
- ✅ domain/primitives/Error (Validation, NotImplemented 포함)
- ✅ domain/primitives/Expected
- ✅ abstractions/infrastructure/logging/ILogger
- ✅ adapters/platform/win32/logging/Win32Logger (log/log.txt)

#### Phase 2: Domain Entities
- ✅ domain/valueobjects (BusType, DiskType, FileSystemType 등)
- ✅ domain/entities/SystemInfo (메타정보만, DiskInfo/VolumeInfo 분리)
- ✅ domain/entities/DiskInfo (Aggregate Root, PartitionInfo 직접 소유)
- ✅ domain/entities/SetupConfig

#### Phase 3: 이벤트 시스템
- ✅ domain/events/DomainEvent + DomainEventBase\<T\>
- ✅ domain/events/DiskAnalyzedEvent
- ✅ domain/events/ErrorOccurredEvent
- ✅ domain/events/InstallCompletedEvent
- ✅ domain/events/InstallProgressEvent
- ✅ domain/events/InstallStartedEvent
- ✅ abstractions/infrastructure/messaging/IEvent (using 별칭)

#### Phase 4: Configuration System
- ✅ adapters/persistence/config/IniParser
- ✅ adapters/persistence/config/IniConfigRepository
- ✅ abstractions/usecases/ILoadConfigurationUseCase
- ✅ application/usecases/system/LoadConfigurationUseCase

#### Phase 5: System Analysis
- ✅ abstractions/usecases/IAnalyzeSystemUseCase (SystemAnalysisResult 포함)
- ✅ application/dto/SystemAnalysisResult (using 별칭)
- ✅ application/usecases/system/AnalyzeSystemUseCase

#### Phase 6: MVVM Pattern
- ✅ abstractions/ui/IPropertyChanged
- ✅ abstractions/ui/IMainViewModel
- ✅ application/viewmodels/MainViewModel
- ✅ adapters/ui/win32/panels/StatusPanel
- ✅ adapters/ui/win32/panels/ActionPanel
- ✅ adapters/ui/win32/panels/OptionPanel
- ✅ adapters/ui/win32/controls/TypeSelectorGroup
- ✅ adapters/ui/win32/Win32MainWindow
- ✅ adapters/ui/win32/Win32ProgressBar

#### Phase 7: DI Container & Composition Root
- ✅ application/core/DIContainer
- ✅ main/ServiceRegistration (ResolveOrThrow 적용)

#### Phase 8: Win32 Core Infrastructure
- ✅ UniqueHandle / UniqueFindHandle / UniqueLibrary
- ✅ Win32Logger
- ✅ Win32HandleFactory
- ✅ Win32SystemInfoService + SMBIOSParser
- ✅ Win32FileCopyService
- ✅ Win32VolumeService

### 📋 미완료 Phase

#### Phase 9: Storage Adapters
- [ ] IOCTLWrapper + AsyncIOCTL
- [ ] Win32DiskService
- [ ] MFTScanner
- [ ] DiskTransaction

#### Phase 10: Imaging Services
- [ ] WimlibAdapter (ApplyImage / CaptureImage / GetImageInfo 실구현)
- [ ] DismAdapter

#### Phase 11: Win32 Core 잔여
- [ ] Win32TypeMapper
- [ ] Win32ErrorHandler
- [ ] Win32StringHelper

#### Phase 12: 나머지 Use Cases
- [ ] InstallWindowsUseCase
- [ ] BackupUserDataUseCase
- [ ] RestoreUserDataUseCase
- [ ] 기타 UseCases

#### Phase 13: BitLocker
- [ ] domain/entities/SetupConfig — `bitLockerEnabled`, `bitLockerPin` 필드 추가
- [ ] adapters/persistence/config/IniConfigRepository — `[BITLOCKER]` 섹션 파싱 추가
- [ ] abstractions/services/security/IBitLockerService.h
- [ ] abstractions/usecases/IConfigureBitLockerUseCase.h
- [ ] application/usecases/system/ConfigureBitLockerUseCase.h / .cpp
- [ ] adapters/platform/win32/security/FveApiBitLockerService.h / .cpp
- [ ] tests/application/usecases/ConfigureBitLockerUseCaseTests.cpp
- [ ] tests/mocks/MockBitLockerService.h

---

## 📊 진행률

### 전체 아키텍처
- Domain 계층: **75%** (primitives, entities, events 완료 / storage 미완)
- Abstractions 계층: **65%** (주요 인터페이스 완료 / security·storage 미완)
- Application 계층: **60%** (MVVM + DI + Config + SystemAnalysis 완료)
- Adapters 계층: **50%** (로깅, Config, UI 전체, SystemInfo, FileCopy, Volume 완료 / IOCTL·Imaging·Security 미완)
- Main 계층: **100%**

### 기능별
- ✅ 로깅: 100%
- ✅ Config 로드: 100%
- ✅ 이벤트 시스템: 100%
- ✅ MVVM: 100%
- ✅ DI Container: 100%
- ✅ 시스템 분석: 100%
- ✅ 파일 복사: 100%
- ✅ 볼륨 열거: 100%
- ✅ UI 컨트롤 (GDI 리소스 관리 포함): 100%
- ⏳ 디스크 서비스 (IOCTL): 0%
- ⏳ 이미징 (wimlib 실구현): 0%
- ⏳ BitLocker (FveAPI): 0%

---

## 🎯 현재 작동 확인된 기능

1. **로깅 시스템**
```
[INIT] Win32Logger initialized
2026-02-18 03:52:43.202 [DEBUG] Window WM_CREATE received
2026-02-18 03:52:43.222 [INFO ] Main window created successfully
```

2. **Config.ini 로드**
```
2026-02-18 03:52:43.222 [INFO ] Loading configuration from: config.ini
2026-02-18 03:52:43.235 [INFO ] Configuration loaded successfully
2026-02-18 03:52:43.235 [INFO ]   User Profile: kdic
2026-02-18 03:52:43.235 [INFO ]   Data Partition: Yes
2026-02-18 03:52:43.235 [INFO ]   Backup Targets: 2
2026-02-18 03:52:43.235 [INFO ]   Installation Types: 4
```

3. **시스템 분석**
```
2026-02-18 03:52:43.240 [INFO ] AnalyzeSystemUseCase: Analysis started.
2026-02-18 03:52:43.251 [INFO ] AnalyzeSystemUseCase: Motherboard = 15Z90S
2026-02-18 03:52:43.252 [INFO ] AnalyzeSystemUseCase: BIOS = T2ZF0200
2026-02-18 03:52:43.252 [INFO ] AnalyzeSystemUseCase: UEFI = true
2026-02-18 03:52:43.253 [INFO ] AnalyzeSystemUseCase: Memory = 16384 MB
2026-02-18 03:52:43.253 [INFO ] AnalyzeSystemUseCase: Analysis complete.
```

4. **MVVM + DI**
   - MainViewModel 초기화 성공
   - PropertyChanged 이벤트 전파
   - View-ViewModel 바인딩 작동
   - 서비스 자동 등록 및 의존성 주입

---

**계획서 작성일**: 2026년 2월 23일
**버전**: 1.0
**상태**: 진행 중 (Phase 9)
