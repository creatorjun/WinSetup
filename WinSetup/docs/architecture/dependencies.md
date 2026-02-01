<!-- docs/architecture/dependencies.md -->

# 의존성 규칙 (Dependency Rules)

## 의존성 규칙의 핵심 원칙

**소스 코드 의존성은 항상 내부를 향해야 합니다.**

```
외부 계층 → 내부 계층 (❌ 금지)
내부 계층 → 외부 계층 (❌ 금지)
내부 계층 → 추상화 (✅ 허용)
외부 계층 → 추상화 구현 (✅ 허용)
```

## 의존성 방향

### 올바른 의존성 흐름

```
Infrastructure (Layer 4)
    ↓ (uses)
Adapters (Layer 3)
    ↓ (implements)
Abstractions (Layer 0) ← Application (Layer 2)
    ↑ (uses)           ↓ (uses)
Domain (Layer 1) ←─────┘
```

### 계층별 의존성 제한

#### Layer 0 (Abstractions)
- **의존 가능**: Domain 타입만
- **의존 불가**: 모든 외부 계층
- **예외**: 없음

#### Layer 1 (Domain)
- **의존 가능**: 표준 C++ 라이브러리, Abstractions 타입
- **의존 불가**: Application, Adapters, Infrastructure
- **예외**: 없음

#### Layer 2 (Application)
- **의존 가능**: Abstractions, Domain
- **의존 불가**: Adapters, Infrastructure
- **예외**: 없음

#### Layer 3 (Adapters)
- **의존 가능**: Abstractions, Domain
- **의존 불가**: Application, Infrastructure
- **예외**: Platform API (Windows API 등)

#### Layer 4 (Infrastructure)
- **의존 가능**: 모든 계층
- **의존 불가**: 없음
- **예외**: 모든 외부 라이브러리 사용 가능

## 의존성 역전 원칙 (DIP)

### 잘못된 예: 구체 클래스 의존

```cpp
class InstallService {
    Win32FileSystem fileSystem;
    Win32ThreadPool threadPool;
    
public:
    void Install() {
        fileSystem.CreateDirectory(L"C:\\Program Files\\MyApp");
        threadPool.Enqueue([]{ });
    }
};
```

**문제점**:
- `InstallService`가 구체적 구현에 직접 의존
- 테스트가 어려움
- Windows 플랫폼에 종속
- 구현 변경 시 `InstallService`도 수정 필요

### 올바른 예: 인터페이스 의존

```cpp
class InstallService {
    IFileSystem& fileSystem;
    IThreadPool& threadPool;
    
public:
    InstallService(IFileSystem& fs, IThreadPool& tp)
        : fileSystem(fs), threadPool(tp) {}
    
    void Install() {
        fileSystem.CreateDirectory(L"C:\\Program Files\\MyApp");
        threadPool.Enqueue([]{ });
    }
};
```

**장점**:
- 인터페이스에만 의존
- Mock을 사용한 테스트 가능
- 플랫폼 독립적
- 런타임에 구현 교체 가능

## 의존성 주입 패턴

### Constructor Injection

```cpp
class DiskAnalyzer {
    IFileSystem& fileSystem;
    IEventBus& eventBus;
    
public:
    DiskAnalyzer(IFileSystem& fs, IEventBus& eb)
        : fileSystem(fs), eventBus(eb) {}
};
```

### Factory Pattern

```cpp
class Win32Factory {
public:
    static std::unique_ptr<IFileSystem> CreateFileSystem() {
        return std::make_unique<Win32FileSystem>();
    }
    
    static std::unique_ptr<IThreadPool> CreateThreadPool(size_t threads) {
        return std::make_unique<Win32ThreadPool>(threads);
    }
};
```

### Dependency Container

```cpp
class DependencyContainer {
    std::unordered_map<std::type_index, std::any> services;
    
public:
    template<typename TInterface, typename TImplementation>
    void Register() {
        services[typeid(TInterface)] = std::make_any<std::shared_ptr<TImplementation>>(
            std::make_shared<TImplementation>()
        );
    }
    
    template<typename TInterface>
    std::shared_ptr<TInterface> Resolve() {
        return std::any_cast<std::shared_ptr<TInterface>>(services[typeid(TInterface)]);
    }
};
```

## 순환 의존성 방지

### 금지: 순환 의존

```cpp
class A {
    B& b;
};

class B {
    A& a;
};
```

### 해결: 인터페이스 분리

```cpp
class IA {
    virtual void MethodA() = 0;
};

class IB {
    virtual void MethodB() = 0;
};

class A : public IA {
    IB& b;
};

class B : public IB {
    IA& a;
};
```

## Include 의존성 관리

### 헤더 파일 구조

```cpp
#pragma once

#include <abstractions/io/IFileSystem.h>
#include <domain/primitives/Result.h>

class FileManager {
    IFileSystem& fileSystem;
    
public:
    Result<void> CreateProjectDirectory();
};
```

### 전방 선언 사용

```cpp
#pragma once

class IFileSystem;
class IEventBus;

class DiskScanner {
    IFileSystem* fileSystem;
    IEventBus* eventBus;
    
public:
    DiskScanner(IFileSystem* fs, IEventBus* eb);
};
```

## 프로젝트 참조 규칙

### Visual Studio 프로젝트 의존성

```
Infrastructure.vcxproj
  ├─ References: Adapters, Application, Domain, Abstractions
  
Adapters.vcxproj
  ├─ References: Abstractions, Domain
  
Application.vcxproj
  ├─ References: Abstractions, Domain
  
Domain.vcxproj
  ├─ References: Abstractions
  
Abstractions.vcxproj
  └─ References: (none)
```

### 컴파일 순서

1. Abstractions (인터페이스 정의)
2. Domain (비즈니스 로직)
3. Application, Adapters (병렬 가능)
4. Infrastructure (최종 조립)

## 테스트 의존성

### 단위 테스트

```cpp
TEST(DiskAnalyzerTest, AnalyzesSystemDisk) {
    MockFileSystem mockFs;
    MockEventBus mockEventBus;
    
    DiskAnalyzer analyzer(mockFs, mockEventBus);
    
    auto result = analyzer.AnalyzeSystemDisk();
    
    ASSERT_TRUE(result.IsSuccess());
}
```

**규칙**:
- 테스트는 Mock 구현만 사용
- 실제 구현 클래스에 의존하지 않음
- 인터페이스를 통한 테스트

## 의존성 검증

### 정적 분석 도구 활용

```powershell
scripts/analyze-dependencies.ps1
```

### 수동 검증 체크리스트

- [ ] Domain 계층이 외부 라이브러리를 참조하지 않는가?
- [ ] Application 계층이 Adapters를 직접 사용하지 않는가?
- [ ] Abstractions 계층에 구현 코드가 없는가?
- [ ] 모든 인터페이스가 순수 가상 함수로만 구성되었는가?
- [ ] Infrastructure만 구체 클래스를 생성하는가?

## 정리

의존성 규칙을 엄격히 준수하면:
- 테스트 가능한 코드
- 유지보수 용이성
- 변경의 영향 최소화
- 모듈 독립성 확보
