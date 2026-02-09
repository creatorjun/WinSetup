다음 구현 추천 순서입니다:

## 🎯 우선순위 1: Domain 계층 완성 (기반 확립)

1. **Value Objects** - 타입 안전성 확보
   - `DiskSize.h/cpp` - 디스크 크기 계산
   - `BusType.h` - NVMe/SATA/USB 구분
   - `FileSystemType.h` - NTFS/FAT32/exFAT
   - `PartitionType.h` - System/EFI/MSR/Data
   - `DriveLetter.h/cpp` - 드라이브 문자 검증

2. **Entities** - 도메인 모델 완성
   - `VolumeInfo.h/cpp` - 볼륨 정보
   - `PartitionInfo.h/cpp` - 파티션 정보
   - `SystemInfo.h/cpp` - 시스템 전체 정보
   - `SetupConfig.h/cpp` - 설정 정보

3. **Domain Services** - 비즈니스 로직
   - `DiskSortingService.h/cpp` - 디스크 정렬 (NVMe > SSD > HDD)
   - `PartitionAnalyzer.h/cpp` - 파티션 분석
   - `PathNormalizer.h/cpp` - 경로 정규화

## 🎯 우선순위 2: Win32 저수준 구현 (핵심 기능)

4. **IOCTLWrapper** - 디스크 제어
   - `IOCTLWrapper.h/cpp` - IOCTL 래퍼 (동기)
   - `AsyncIOCTL.h/cpp` - IOCTL 비동기 처리

5. **DiskTransaction** - 트랜잭션
   - `DiskTransaction.h/cpp` - 원자성 보장 + 롤백

6. **Win32TypeMapper** - 타입 변환
   - `Win32TypeMapper.h/cpp` - Win32 ↔ Domain 변환

7. **Win32ErrorHandler** - 에러 처리
   - `Win32ErrorHandler.h/cpp` - GetLastError() → Error 변환

## 🎯 우선순위 3: Use Cases (애플리케이션 로직)

8. **System Use Cases**
   - `AnalyzeSystemUseCase.h/cpp` - 시스템 분석
   - `LoadConfigurationUseCase.h/cpp` - 설정 로드

9. **Disk Use Cases**
   - `EnumerateDisksUseCase.h/cpp` - 디스크 열거
   - `AnalyzeDisksUseCase.h/cpp` - 디스크 분석
   - `SelectTargetDisksUseCase.h/cpp` - 대상 디스크 선택

10. **Install Use Cases**
    - `InstallWindowsUseCase.h/cpp` - Windows 설치 오케스트레이션
    - `ApplyImageUseCase.h/cpp` - WIM 이미지 적용
    - `InjectDriversUseCase.h/cpp` - 드라이버 주입

## 🎯 우선순위 4: Event System (관찰 가능성)

11. **Domain Events**
    - `DomainEvent.h/cpp` - 이벤트 베이스
    - `DiskAnalyzedEvent.h` - 디스크 분석 완료
    - `InstallProgressEvent.h` - 설치 진행률

12. **Event Infrastructure**
    - `EventBus.h/cpp` - 이벤트 버스
    - `Dispatcher.h/cpp` - 이벤트 디스패처

## 🎯 우선순위 5: UI 구현 (사용자 인터페이스)

13. **Win32 UI Components**
    - `Win32MainWindow.h/cpp` - 메인 윈도우
    - `Win32ProgressBar.h/cpp` - 진행률 표시
    - `SimpleButton.h/cpp` - 버튼 컨트롤
    - `ToggleButton.h/cpp` - 토글 버튼

## 🎯 우선순위 6: 고급 기능

14. **Imaging Adapters**
    - `WimlibAdapter.h/cpp` - wimlib 래퍼
    - `WimlibOptimizer.h/cpp` - WIM 최적화

15. **Persistence**
    - `IniConfigRepository.h/cpp` - INI 파일 저장소
    - `Win32FileSystem.h/cpp` - 파일 시스템 추상화

16. **Threading**
    - `Win32ThreadPool.h/cpp` - 스레드 풀
    - `TaskScheduler.h/cpp` - 태스크 스케줄러

## 📋 권장 구현 순서 요약

```
1단계: Domain 완성 (1-3)           ← 타입 안전성 확보
2단계: Win32 저수준 (4-7)          ← 디스크 제어 구현
3단계: Use Cases (8-10)            ← 비즈니스 로직
4단계: Event System (11-12)        ← 옵저버 패턴
5단계: UI (13)                      ← 사용자 인터페이스
6단계: 고급 기능 (14-16)           ← 이미징, 멀티스레딩
```

**다음 대화에서 어떤 그룹부터 구현할까요?** (1-3번 Domain 완성을 추천합니다)