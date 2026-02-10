1️⃣ Win32TypeMapper (최우선)
Windows API 타입을 도메인 타입으로 변환하는 핵심 어댑터

BusType, DiskType 등을 Windows의 STORAGE_BUS_TYPE, MEDIA_TYPE에서 변환

Win32DiskService가 실제 디스크 정보를 제대로 가져오려면 필수

2️⃣ VolumeInfo 엔티티
Win32VolumeService에서 사용

볼륨 정보 관리 (드라이브 레터, 파일 시스템, 크기 등)

이미 헤더 파일에 정의되어 있지만 cpp 구현 필요

3️⃣ SystemInfo 엔티티
시스템 전체 정보를 담는 집합체

디스크/볼륨 정보를 모아서 관리

UseCase에서 사용할 핵심 데이터 구조

4️⃣ Use Cases 구현
EnumerateDisksUseCase - 디스크 목록 조회

AnalyzeDisksUseCase - 디스크 분석

AnalyzeSystemUseCase - 시스템 분석

