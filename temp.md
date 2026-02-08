클린 아키텍처 의존성 규칙에 따라 **다음 구현 우선순위**를 제안합니다:

***

## 📊 다음 구현 우선순위

### 🎯 **1단계: Domain Entities & ValueObjects** (가장 우선)
**이유:** 모든 계층에서 사용되는 데이터 구조 정의

```
src/domain/
├── valueobjects/
│   ├── BusType.h              # enum (NVME, SATA, HDD 등)
│   ├── DiskSize.h/.cpp        # 크기 계산 (GB 변환)
│   ├── FileSystemType.h       # enum (NTFS, FAT32, exFAT)
│   ├── PartitionType.h        # enum (System, EFI, MSR, Basic)
│   └── DriveLetter.h/.cpp     # 드라이브 문자 검증
│
└── entities/
    ├── DiskInfo.h/.cpp        # 디스크 정보 (Index, Type, Size, Volumes)
    ├── VolumeInfo.h/.cpp      # 볼륨 정보 (Letter, Label, FileSystem)
    ├── PartitionInfo.h/.cpp   # 파티션 정보 (Offset, Size, Type)
    ├── SystemInfo.h/.cpp      # 시스템 정보 (Mainboard, Disks)
    └── SetupConfig.h/.cpp     # Config.ini 데이터
```

**핵심 이유:**
- ✅ 외부 의존성 0 (순수 C++)
- ✅ 모든 상위 계층에서 참조
- ✅ 타입 안전성 보장

***

### 🎯 **2단계: Abstractions - Core Interfaces**
**이유:** Domain을 사용하는 인터페이스 정의

```
src/abstractions/
├── repositories/
│   ├── IConfigRepository.h    # Config.ini 읽기/쓰기
│   ├── IDiskRepository.h      # 디스크 정보 저장소
│   └── IVolumeRepository.h    # 볼륨 정보 저장소
│
└── services/
    ├── storage/
    │   ├── IDiskService.h          # 디스크 조작 (Clean, Partition, Format)
    │   ├── IVolumeService.h        # 볼륨 조작 (Assign Letter)
    │   ├── IStorageScanner.h       # 디스크/볼륨 열거
    │   └── IPartitionService.h     # 파티션 생성/삭제
    │
    └── platform/
        ├── ISystemInfoService.h    # SMBIOS 읽기
        └── ITextEncoder.h          # UTF-8 ↔ Wide 변환
```

**핵심 이유:**
- ✅ 구현과 인터페이스 분리
- ✅ 테스트 가능성 확보 (Mock 생성)

***

### 🎯 **3단계: Adapters - Win32 기본 서비스**
**이유:** 인터페이스의 실제 구현

```
src/adapters/platform/win32/
├── core/
│   ├── Win32TypeMapper.h/.cpp      # DWORD → domain 타입 변환
│   └── Win32ErrorHandler.h/.cpp    # GetLastError → Error 변환
│
├── system/
│   ├── Win32SystemInfoService.h/.cpp   # SMBIOS 파싱
│   └── SMBIOSParser.h/.cpp             # 펌웨어 테이블 읽기
│
└── storage/
    ├── IOCTLWrapper.h/.cpp             # IOCTL 호출 래퍼
    └── Win32DiskService.h/.cpp         # 디스크 서비스 구현 (기본)
```

**핵심 이유:**
- ✅ 저수준 Win32 API 캡슐화
- ✅ Domain 타입으로 변환
- ✅ Expected<T> 반환

***

### 🎯 **4단계: Adapters - 고급 스토리지 기능**
**이유:** 핵심 성능 최적화 구현

```
src/adapters/platform/win32/storage/
├── AsyncIOCTL.h/.cpp          # OVERLAPPED 비동기 I/O
├── MFTScanner.h/.cpp          # FSCTL_ENUM_USN_DATA로 MFT 읽기
├── DiskTransaction.h/.cpp     # 트랜잭션 + 롤백
└── DiskLayoutBuilder.h/.cpp   # GPT/MBR 레이아웃 생성
```

**핵심 이유:**
- ⚡ 성능 최적화 (병렬 IOCTL, MFT 직접 읽기)
- 🔒 안전성 (트랜잭션)

***

### 🎯 **5단계: Application - Use Cases**
**이유:** 비즈니스 워크플로우 구현

```
src/application/usecases/
├── system/
│   ├── AnalyzeSystemUseCase.h/.cpp     # Step 1: SMBIOS + Config
│   └── LoadConfigurationUseCase.h/.cpp
│
├── disk/
│   ├── EnumerateDisksUseCase.h/.cpp    # Step 2: 디스크 열거
│   ├── AnalyzeDisksUseCase.h/.cpp      # MFT 분석
│   └── SelectTargetDisksUseCase.h/.cpp
│
└── install/
    ├── InstallWindowsUseCase.h/.cpp    # Step 4: 전체 설치 흐름
    ├── ApplyImageUseCase.h/.cpp        # wimlib 적용
    └── InjectDriversUseCase.h/.cpp     # DismApi 주입
```

**핵심 이유:**
- 📋 비즈니스 로직 중앙화
- 🔄 재사용 가능
- ✅ 단위 테스트 가능

***

### 🎯 **6단계: UI 통합**
**이유:** 모든 백엔드 완성 후

```
src/adapters/ui/win32/
├── Win32MainWindow.h/.cpp     # 메인 윈도우
├── panels/
│   ├── TypeButtonPanel.h/.cpp
│   ├── LogPanel.h/.cpp
│   └── StartStopPanel.h/.cpp
│
└── viewmodels/
    └── MainViewModel.h/.cpp   # UI ↔ UseCase 연결
```

***

## 🎯 **추천 구현 순서**

```
1️⃣ Domain Entities (DiskInfo, VolumeInfo, SystemInfo)
   → 데이터 구조 확정

2️⃣ Abstractions Interfaces (IDiskService, IStorageScanner)
   → 계약 정의

3️⃣ Adapters Win32 기본 (IOCTLWrapper, Win32DiskService)
   → 실제 동작 구현

4️⃣ Adapters 고급 (AsyncIOCTL, MFTScanner)
   → 최적화 추가

5️⃣ Application Use Cases (EnumerateDisksUseCase)
   → 비즈니스 로직

6️⃣ UI Integration
   → 사용자 인터페이스
```

***