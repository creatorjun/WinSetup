<!-- docs/architecture/layers.md -->

# 계층 구조 상세 (Layer Architecture)

## 개요

WinSetup 프로젝트는 클린 아키텍처의 5계층 구조를 따릅니다. 각 계층은 명확한 책임과 의존성 규칙을 가지며, 소스 코드 의존성은 항상 내부를 향합니다.

## 계층 다이어그램

```
┌─────────────────────────────────────────────────────────────┐
│                    Layer 4: Infrastructure                  │
│                  (Frameworks & Drivers)                     │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Windows API, File System, Registry, Network           │  │
│  │ - 실제 OS 호출                                         │  │
│  │ - UI Framework                                        │  │
│  │ - 외부 라이브러리                                       │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ (구현)
┌─────────────────────────────────────────────────────────────┐
│                    Layer 3: Adapters                        │
│                  (Interface Adapters)                       │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Platform Adapters, Controllers, Presenters            │  │
│  │ - Win32TextEncoder                                    │  │
│  │ - Win32ThreadPool                                     │  │
│  │ - Win32FileSystem                                     │  │
│  │ - Win32WindowHandle                                   │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ (의존)
┌─────────────────────────────────────────────────────────────┐
│                    Layer 2: Application                     │
│                  (Use Cases / Business Logic)               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Application Services, Use Cases                       │  │
│  │ - EventBus                                            │  │
│  │ - Dispatcher                                          │  │
│  │ - Task / AsyncContext                                 │  │
│  │ - Property (Reactive)                                 │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ (의존)
┌─────────────────────────────────────────────────────────────┐
│                    Layer 1: Domain                          │
│                  (Entities / Business Rules)                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Domain Entities, Value Objects, Domain Services       │  │
│  │ - Expected / Result                                   │  │
│  │ - Functional (Monads, Optional)                       │  │
│  │ - Memory (SmartPtr, UniqueResource)                   │  │
│  │ - PathValidator (비즈니스 규칙)                         │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ (의존)
┌─────────────────────────────────────────────────────────────┐
│                    Layer 0: Abstractions                    │
│                  (Interfaces / Contracts)                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Pure Interfaces - 구현 없음                            │  │
│  │ - ITextEncoder, IFileSystem                           │  │
│  │ - IThreadPool, IWindowHandle                          │  │
│  │ - IEventBus, IDispatcher                              │  │
│  │ - IObservable, IProperty                              │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Layer 0: Abstractions (추상화)

### 역할
모든 인터페이스 정의를 담당하는 최내부 계층입니다.

### 의존성
- Domain 계층의 타입만 의존 (예: `Result<T>`, `Expected<T>`)
- 다른 계층에 대한 의존성 없음

### 규칙
- 순수 가상 함수만 포함
- 구현 코드 절대 금지
- Platform 독립적
- 외부 라이브러리 사용 금지

### 주요 컴포넌트
- **platform/**: `ITextEncoder`, `IThreadPool`, `IWindowHandle`
- **io/**: `IFileSystem`, `IStreamReader`, `IStreamWriter`
- **async/**: `IExecutor`, `IScheduler`, `IAsyncContext`
- **messaging/**: `IEventBus`, `IDispatcher`, `IMessageQueue`
- **reactive/**: `IObservable`, `IProperty`

### 예제

```cpp
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual Result<void> CreateDirectory(const std::wstring& path) = 0;
    virtual Expected<std::vector<std::wstring>, Error> ListFiles(const std::wstring& path) = 0;
};
```

## Layer 1: Domain (도메인)

### 역할
핵심 비즈니스 규칙과 엔티티를 정의합니다.

### 의존성
- 외부 의존성 0 (순수 C++ 표준 라이브러리만)
- Abstractions 계층 타입 사용 가능

### 규칙
- 외부 라이브러리 사용 금지
- OS API 호출 금지
- 불변성 보장
- Platform 독립적

### 주요 컴포넌트
- **primitives/**: `Expected<T>`, `Result<T>`, `Error`
- **functional/**: `Monads`, `Optional`, `Pipeline`
- **memory/**: `SmartPtr`, `UniqueResource`, `PoolAllocator`
- **validation/**: `PathValidator`, `ValidationRules`

### 예제

```cpp
template<typename T, typename E = Error>
class Expected {
    std::variant<T, E> data;
public:
    bool HasValue() const;
    const T& Value() const;
    const E& Error() const;
};
```

## Layer 2: Application (애플리케이션)

### 역할
유즈케이스와 애플리케이션 로직을 구현합니다.

### 의존성
- Abstractions 계층의 인터페이스
- Domain 계층의 엔티티와 값 객체

### 규칙
- 인터페이스에만 의존
- Platform 독립적
- 비즈니스 플로우 조율
- 구체적 구현 클래스 의존 금지

### 주요 컴포넌트
- **async/**: `Task`, `TaskScheduler`, `AsyncContext`
- **messaging/**: `EventBus`, `Dispatcher`, `MessageQueue`
- **reactive/**: `Property`, `Observable`, `Subject`
- **events/**: `LogEvent`, `ProgressEvent`, `ErrorEvent`

### 예제

```cpp
class TaskScheduler {
    IExecutor& executor;
    IDispatcher& dispatcher;
public:
    TaskScheduler(IExecutor& exec, IDispatcher& disp);
    Task<void> Schedule(std::function<void()> work);
};
```

## Layer 3: Adapters (어댑터)

### 역할
외부 세계(OS, 라이브러리)와 내부 계층을 연결합니다.

### 의존성
- Abstractions 계층의 인터페이스 구현
- Domain 계층의 타입 사용

### 규칙
- 인터페이스 구현
- Platform 특정 코드 허용
- Application 계층을 알지 못함
- Windows API 직접 사용 가능

### 주요 컴포넌트
- **encoding/**: `Win32TextEncoder`
- **threading/**: `Win32ThreadPool`, `Win32Thread`
- **window/**: `Win32WindowHandle`, `Win32Handle`
- **filesystem/**: `Win32FileSystem`, `Win32File`

### 예제

```cpp
class Win32FileSystem : public IFileSystem {
    Result<void> CreateDirectory(const std::wstring& path) override {
        if (::CreateDirectoryW(path.c_str(), nullptr)) {
            return Result<void>::Success();
        }
        return Result<void>::Failure(GetLastError());
    }
};
```

## Layer 4: Infrastructure (인프라)

### 역할
프레임워크, 드라이버, UI, Composition Root를 담당합니다.

### 의존성
- 모든 계층 사용 가능
- 외부 라이브러리 사용 가능

### 규칙
- Composition Root (main.cpp)
- 의존성 주입 설정
- 모든 컴포넌트 연결
- 애플리케이션 진입점

### 주요 컴포넌트
- **composition/**: `DependencyContainer`, `ServiceLocator`
- **logging/**: `WindowsLogger`
- **config/**: `ConfigLoader`
- **diagnostics/**: `PerformanceMonitor`

### 예제

```cpp
int main() {
    DependencyContainer container;
    
    auto fileSystem = std::make_unique<Win32FileSystem>();
    auto threadPool = std::make_unique<Win32ThreadPool>(8);
    
    container.Register<IFileSystem>(std::move(fileSystem));
    container.Register<IThreadPool>(std::move(threadPool));
    
    auto app = container.Resolve<Application>();
    return app->Run();
}
```

## 계층 간 통신 규칙

### 허용되는 의존성
- Infrastructure → Adapters → Application → Domain → Abstractions
- 모든 계층 → Abstractions

### 금지되는 의존성
- Domain → Application (❌)
- Application → Adapters (❌)
- Adapters → Infrastructure (❌)

## 정리

각 계층은 명확한 책임과 경계를 가지며, 의존성 규칙을 엄격히 준수해야 합니다. 이를 통해 테스트 가능하고 유지보수 가능한 시스템을 구축할 수 있습니다.
