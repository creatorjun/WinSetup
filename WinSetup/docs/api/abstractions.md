<!-- docs/api/abstractions.md -->

# Abstractions API Reference

## 개요

Layer 0 Abstractions는 모든 인터페이스를 정의하는 계층입니다. 구현 코드 없이 순수 가상 함수만 포함합니다.

## Platform Abstractions

### ITextEncoder

문자열 인코딩 변환을 위한 인터페이스입니다.

```cpp
class ITextEncoder {
public:
    virtual ~ITextEncoder() = default;
    
    virtual Expected<std::wstring, Error> ToWide(const std::string& utf8) = 0;
    virtual Expected<std::string, Error> ToUtf8(const std::wstring& wide) = 0;
    virtual Expected<std::string, Error> ToAnsi(const std::wstring& wide) = 0;
};
```

#### 메서드

**ToWide**
- UTF-8 문자열을 UTF-16 와이드 문자열로 변환
- 매개변수: `utf8` - UTF-8 인코딩 문자열
- 반환: 변환된 와이드 문자열 또는 에러

**ToUtf8**
- UTF-16 와이드 문자열을 UTF-8로 변환
- 매개변수: `wide` - UTF-16 와이드 문자열
- 반환: 변환된 UTF-8 문자열 또는 에러

**ToAnsi**
- UTF-16 와이드 문자열을 ANSI로 변환
- 매개변수: `wide` - UTF-16 와이드 문자열
- 반환: 변환된 ANSI 문자열 또는 에러

#### 사용 예제

```cpp
void ProcessText(ITextEncoder& encoder) {
    auto wide = encoder.ToWide(u8"안녕하세요");
    if (wide.HasValue()) {
        std::wcout << wide.Value() << std::endl;
    }
}
```

### IThreadPool

스레드 풀 관리를 위한 인터페이스입니다.

```cpp
class IThreadPool {
public:
    virtual ~IThreadPool() = default;
    
    virtual void Enqueue(std::function<void()> work) = 0;
    virtual void WaitAll() = 0;
    virtual size_t GetThreadCount() const = 0;
};
```

#### 메서드

**Enqueue**
- 작업을 스레드 풀에 추가
- 매개변수: `work` - 실행할 작업 함수

**WaitAll**
- 모든 작업이 완료될 때까지 대기

**GetThreadCount**
- 스레드 풀의 스레드 개수 반환

#### 사용 예제

```cpp
void ProcessParallel(IThreadPool& pool) {
    for (int i = 0; i < 100; ++i) {
        pool.Enqueue([i] {
            std::cout << "Processing " << i << std::endl;
        });
    }
    pool.WaitAll();
}
```

### IWindowHandle

윈도우 핸들 관리를 위한 인터페이스입니다.

```cpp
class IWindowHandle {
public:
    virtual ~IWindowHandle() = default;
    
    virtual void* GetNativeHandle() const = 0;
    virtual Result<void> Show() = 0;
    virtual Result<void> Hide() = 0;
    virtual Result<void> SetText(const std::wstring& text) = 0;
};
```

## IO Abstractions

### IFileSystem

파일 시스템 작업을 위한 인터페이스입니다.

```cpp
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    
    virtual Result<void> CreateDirectory(const std::wstring& path) = 0;
    virtual Result<void> DeleteDirectory(const std::wstring& path) = 0;
    virtual Result<void> DeleteFile(const std::wstring& path) = 0;
    
    virtual Expected<std::vector<std::wstring>, Error> ListFiles(
        const std::wstring& path) = 0;
    virtual Expected<std::vector<std::wstring>, Error> ListDirectories(
        const std::wstring& path) = 0;
    
    virtual Expected<bool, Error> FileExists(const std::wstring& path) = 0;
    virtual Expected<bool, Error> DirectoryExists(const std::wstring& path) = 0;
    
    virtual Expected<uint64_t, Error> GetFileSize(const std::wstring& path) = 0;
};
```

#### 사용 예제

```cpp
void AnalyzeDirectory(IFileSystem& fs, const std::wstring& path) {
    auto files = fs.ListFiles(path);
    if (files.HasValue()) {
        for (const auto& file : files.Value()) {
            auto size = fs.GetFileSize(path + L"\\" + file);
            if (size.HasValue()) {
                std::wcout << file << L": " << size.Value() << L" bytes\n";
            }
        }
    }
}
```

### IStreamReader

스트림 읽기를 위한 인터페이스입니다.

```cpp
class IStreamReader {
public:
    virtual ~IStreamReader() = default;
    
    virtual Expected<std::vector<uint8_t>, Error> ReadAll() = 0;
    virtual Expected<size_t, Error> Read(uint8_t* buffer, size_t size) = 0;
    virtual Expected<std::string, Error> ReadLine() = 0;
    virtual Result<void> Seek(int64_t offset, SeekOrigin origin) = 0;
};
```

### IStreamWriter

스트림 쓰기를 위한 인터페이스입니다.

```cpp
class IStreamWriter {
public:
    virtual ~IStreamWriter() = default;
    
    virtual Result<void> Write(const uint8_t* buffer, size_t size) = 0;
    virtual Result<void> WriteLine(const std::string& line) = 0;
    virtual Result<void> Flush() = 0;
};
```

## Async Abstractions

### IExecutor

비동기 작업 실행을 위한 인터페이스입니다.

```cpp
class IExecutor {
public:
    virtual ~IExecutor() = default;
    
    virtual void Execute(std::function<void()> work) = 0;
    virtual void ExecuteDelayed(std::function<void()> work, 
                               std::chrono::milliseconds delay) = 0;
};
```

### IScheduler

작업 스케줄링을 위한 인터페이스입니다.

```cpp
class IScheduler {
public:
    virtual ~IScheduler() = default;
    
    virtual Task<void> Schedule(std::function<void()> work) = 0;
    virtual Task<void> ScheduleAt(std::function<void()> work, 
                                  std::chrono::system_clock::time_point time) = 0;
};
```

### IAsyncContext

비동기 컨텍스트 관리를 위한 인터페이스입니다.

```cpp
class IAsyncContext {
public:
    virtual ~IAsyncContext() = default;
    
    virtual void Post(std::function<void()> work) = 0;
    virtual void Run() = 0;
    virtual void Stop() = 0;
};
```

## Messaging Abstractions

### IEventBus

이벤트 발행/구독을 위한 인터페이스입니다.

```cpp
class IEventBus {
public:
    virtual ~IEventBus() = default;
    
    template<typename TEvent>
    virtual void Subscribe(std::function<void(const TEvent&)> handler) = 0;
    
    template<typename TEvent>
    virtual void Unsubscribe(int subscriptionId) = 0;
    
    template<typename TEvent>
    virtual void Publish(const TEvent& event) = 0;
};
```

#### 사용 예제

```cpp
void SetupEventHandlers(IEventBus& eventBus) {
    eventBus.Subscribe<ProgressEvent>([](const ProgressEvent& e) {
        std::wcout << L"Progress: " << e.message << std::endl;
    });
    
    eventBus.Subscribe<ErrorEvent>([](const ErrorEvent& e) {
        std::wcerr << L"Error: " << e.error.Message() << std::endl;
    });
}

void ReportProgress(IEventBus& eventBus, int percent) {
    eventBus.Publish(ProgressEvent(std::format(L"{}%", percent)));
}
```

### IDispatcher

UI 스레드 디스패칭을 위한 인터페이스입니다.

```cpp
class IDispatcher {
public:
    virtual ~IDispatcher() = default;
    
    virtual void Invoke(std::function<void()> work) = 0;
    virtual Task<void> InvokeAsync(std::function<void()> work) = 0;
    virtual bool CheckAccess() const = 0;
};
```

### IMessageQueue

메시지 큐 관리를 위한 인터페이스입니다.

```cpp
class IMessageQueue {
public:
    virtual ~IMessageQueue() = default;
    
    virtual void Enqueue(std::any message) = 0;
    virtual Expected<std::any, Error> Dequeue() = 0;
    virtual size_t GetCount() const = 0;
};
```

## Reactive Abstractions

### IObservable

관찰 가능한 스트림을 위한 인터페이스입니다.

```cpp
template<typename T>
class IObservable {
public:
    virtual ~IObservable() = default;
    
    virtual int Subscribe(std::function<void(const T&)> onNext,
                         std::function<void(const Error&)> onError,
                         std::function<void()> onCompleted) = 0;
    virtual void Unsubscribe(int subscriptionId) = 0;
};
```

#### 사용 예제

```cpp
void ObserveDiskChanges(IObservable<DiskInfo>& observable) {
    observable.Subscribe(
        [](const DiskInfo& disk) {
            std::wcout << L"Disk detected: " << disk.path << std::endl;
        },
        [](const Error& error) {
            std::wcerr << L"Error: " << error.Message() << std::endl;
        },
        []() {
            std::wcout << L"Scan completed" << std::endl;
        }
    );
}
```

### IProperty

반응형 프로퍼티를 위한 인터페이스입니다.

```cpp
template<typename T>
class IProperty {
public:
    virtual ~IProperty() = default;
    
    virtual T Get() const = 0;
    virtual void Set(T value) = 0;
    
    virtual int Subscribe(std::function<void(const T&)> onChange) = 0;
    virtual void Unsubscribe(int subscriptionId) = 0;
};
```

#### 사용 예제

```cpp
void BindProgress(IProperty<int>& progressProperty) {
    progressProperty.Subscribe([](int progress) {
        std::cout << "Progress: " << progress << "%" << std::endl;
    });
    
    progressProperty.Set(0);
    progressProperty.Set(50);
    progressProperty.Set(100);
}
```

## 정리

Abstractions 계층은 모든 구현의 계약을 정의하며, 의존성 역전의 핵심입니다.
