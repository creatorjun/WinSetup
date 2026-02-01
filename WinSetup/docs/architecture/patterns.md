<!-- docs/architecture/patterns.md -->

# 디자인 패턴 (Design Patterns)

## 개요

WinSetup 프로젝트에서 사용되는 주요 디자인 패턴과 아키텍처 패턴을 설명합니다.

## SOLID 원칙

### Single Responsibility Principle (SRP)

각 클래스는 단 하나의 책임만 가져야 합니다.

#### 잘못된 예

```cpp
class UserManager {
    void SaveUser();
    void ValidateUser();
    void SendEmail();
    void LogActivity();
};
```

#### 올바른 예

```cpp
class UserRepository {
    void SaveUser();
};

class UserValidator {
    bool ValidateUser(const User& user);
};

class EmailService {
    void SendEmail(const std::wstring& to, const std::wstring& body);
};

class ActivityLogger {
    void LogActivity(const std::wstring& message);
};
```

### Open/Closed Principle (OCP)

확장에는 열려 있고 수정에는 닫혀 있어야 합니다.

```cpp
class IStorageDevice {
public:
    virtual ~IStorageDevice() = default;
    virtual Result<DiskInfo> GetInfo() = 0;
};

class SSDDevice : public IStorageDevice {
    Result<DiskInfo> GetInfo() override;
};

class HDDDevice : public IStorageDevice {
    Result<DiskInfo> GetInfo() override;
};

class NVMeDevice : public IStorageDevice {
    Result<DiskInfo> GetInfo() override;
};
```

### Liskov Substitution Principle (LSP)

하위 타입은 상위 타입을 완전히 대체할 수 있어야 합니다.

```cpp
void ProcessDisk(IStorageDevice& device) {
    auto info = device.GetInfo();
    if (info.IsSuccess()) {
    }
}

SSDDevice ssd;
HDDDevice hdd;

ProcessDisk(ssd);
ProcessDisk(hdd);
```

### Interface Segregation Principle (ISP)

클라이언트는 사용하지 않는 메서드에 의존하지 않아야 합니다.

#### 잘못된 예

```cpp
class IDataAccess {
    virtual void Read() = 0;
    virtual void Write() = 0;
    virtual void Delete() = 0;
    virtual void Update() = 0;
};
```

#### 올바른 예

```cpp
class IReader {
    virtual void Read() = 0;
};

class IWriter {
    virtual void Write() = 0;
};

class IDeleter {
    virtual void Delete() = 0;
};
```

### Dependency Inversion Principle (DIP)

고수준 모듈은 저수준 모듈에 의존하지 않고, 둘 다 추상화에 의존해야 합니다.

```cpp
class InstallOrchestrator {
    IFileSystem& fileSystem;
    IDiskManager& diskManager;
    
public:
    InstallOrchestrator(IFileSystem& fs, IDiskManager& dm)
        : fileSystem(fs), diskManager(dm) {}
};
```

## 생성 패턴 (Creational Patterns)

### Factory Pattern

```cpp
class Win32Factory {
public:
    static std::unique_ptr<IFileSystem> CreateFileSystem() {
        return std::make_unique<Win32FileSystem>();
    }
    
    static std::unique_ptr<IThreadPool> CreateThreadPool(size_t threadCount) {
        return std::make_unique<Win32ThreadPool>(threadCount);
    }
    
    static std::unique_ptr<IWindowHandle> CreateWindow(HWND hwnd) {
        return std::make_unique<Win32WindowHandle>(hwnd);
    }
};
```

### Builder Pattern

```cpp
class TaskBuilder {
    std::function<void()> work;
    Priority priority = Priority::Normal;
    std::wstring name;
    
public:
    TaskBuilder& SetWork(std::function<void()> w) {
        work = std::move(w);
        return *this;
    }
    
    TaskBuilder& SetPriority(Priority p) {
        priority = p;
        return *this;
    }
    
    TaskBuilder& SetName(std::wstring n) {
        name = std::move(n);
        return *this;
    }
    
    Task Build() {
        return Task(std::move(work), priority, std::move(name));
    }
};

auto task = TaskBuilder()
    .SetWork([]{ })
    .SetPriority(Priority::High)
    .SetName(L"DiskAnalysis")
    .Build();
```

### Singleton Pattern

```cpp
class DependencyContainer {
    static DependencyContainer* instance;
    std::unordered_map<std::type_index, std::any> services;
    
    DependencyContainer() = default;
    
public:
    static DependencyContainer& GetInstance() {
        static DependencyContainer instance;
        return instance;
    }
    
    DependencyContainer(const DependencyContainer&) = delete;
    DependencyContainer& operator=(const DependencyContainer&) = delete;
};
```

## 구조 패턴 (Structural Patterns)

### Adapter Pattern

```cpp
class Win32FileSystem : public IFileSystem {
public:
    Result<void> CreateDirectory(const std::wstring& path) override {
        if (::CreateDirectoryW(path.c_str(), nullptr)) {
            return Result<void>::Success();
        }
        return Result<void>::Failure(Error::FromWin32(GetLastError()));
    }
    
    Expected<std::vector<std::wstring>, Error> ListFiles(
        const std::wstring& path) override {
        WIN32_FIND_DATAW findData;
        HANDLE hFind = ::FindFirstFileW((path + L"\\*").c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return Error::FromWin32(GetLastError());
        }
        
        std::vector<std::wstring> files;
        do {
            files.push_back(findData.cFileName);
        } while (::FindNextFileW(hFind, &findData));
        
        ::FindClose(hFind);
        return files;
    }
};
```

### Facade Pattern

```cpp
class InstallationFacade {
    IFileSystem& fileSystem;
    IDiskManager& diskManager;
    IImageApplier& imageApplier;
    IEventBus& eventBus;
    
public:
    InstallationFacade(IFileSystem& fs, IDiskManager& dm, 
                       IImageApplier& ia, IEventBus& eb)
        : fileSystem(fs), diskManager(dm), imageApplier(ia), eventBus(eb) {}
    
    Task<Result<void>> InstallWindows(const InstallOptions& options) {
        eventBus.Publish(ProgressEvent(L"Analyzing disk..."));
        auto disk = co_await diskManager.SelectSystemDisk();
        
        eventBus.Publish(ProgressEvent(L"Formatting..."));
        co_await diskManager.Format(disk);
        
        eventBus.Publish(ProgressEvent(L"Applying image..."));
        co_await imageApplier.Apply(options.ImagePath, disk);
        
        co_return Result<void>::Success();
    }
};
```

### Proxy Pattern

```cpp
class CachedFileSystem : public IFileSystem {
    IFileSystem& underlying;
    std::unordered_map<std::wstring, std::vector<std::wstring>> cache;
    
public:
    explicit CachedFileSystem(IFileSystem& fs) : underlying(fs) {}
    
    Expected<std::vector<std::wstring>, Error> ListFiles(
        const std::wstring& path) override {
        
        if (auto it = cache.find(path); it != cache.end()) {
            return it->second;
        }
        
        auto result = underlying.ListFiles(path);
        if (result.HasValue()) {
            cache[path] = result.Value();
        }
        return result;
    }
};
```

## 행동 패턴 (Behavioral Patterns)

### Observer Pattern

```cpp
class EventBus : public IEventBus {
    std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> subscribers;
    
public:
    template<typename TEvent>
    void Subscribe(std::function<void(const TEvent&)> handler) {
        subscribers[typeid(TEvent)].push_back([handler](const void* event) {
            handler(*static_cast<const TEvent*>(event));
        });
    }
    
    template<typename TEvent>
    void Publish(const TEvent& event) {
        auto& handlers = subscribers[typeid(TEvent)];
        for (auto& handler : handlers) {
            handler(&event);
        }
    }
};

eventBus.Subscribe<ProgressEvent>([](const ProgressEvent& e) {
    std::wcout << L"Progress: " << e.message << std::endl;
});

eventBus.Publish(ProgressEvent(L"Formatting disk..."));
```

### Strategy Pattern

```cpp
class IDiskSelector {
public:
    virtual ~IDiskSelector() = default;
    virtual Expected<DiskInfo, Error> SelectDisk(
        const std::vector<DiskInfo>& disks) = 0;
};

class FastestDiskSelector : public IDiskSelector {
    Expected<DiskInfo, Error> SelectDisk(
        const std::vector<DiskInfo>& disks) override {
        return *std::max_element(disks.begin(), disks.end(),
            [](const auto& a, const auto& b) { return a.speed < b.speed; });
    }
};

class LargestDiskSelector : public IDiskSelector {
    Expected<DiskInfo, Error> SelectDisk(
        const std::vector<DiskInfo>& disks) override {
        return *std::max_element(disks.begin(), disks.end(),
            [](const auto& a, const auto& b) { return a.size < b.size; });
    }
};

class DiskManager {
    IDiskSelector& selector;
public:
    explicit DiskManager(IDiskSelector& sel) : selector(sel) {}
    
    Expected<DiskInfo, Error> SelectBestDisk(const std::vector<DiskInfo>& disks) {
        return selector.SelectDisk(disks);
    }
};
```

### Command Pattern

```cpp
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual Result<void> Execute() = 0;
    virtual Result<void> Undo() = 0;
};

class FormatDiskCommand : public ICommand {
    IDiskManager& diskManager;
    DiskInfo disk;
    std::vector<uint8_t> backup;
    
public:
    FormatDiskCommand(IDiskManager& dm, const DiskInfo& d)
        : diskManager(dm), disk(d) {}
    
    Result<void> Execute() override {
        backup = diskManager.Backup(disk);
        return diskManager.Format(disk);
    }
    
    Result<void> Undo() override {
        return diskManager.Restore(disk, backup);
    }
};

class CommandInvoker {
    std::stack<std::unique_ptr<ICommand>> history;
    
public:
    Result<void> Execute(std::unique_ptr<ICommand> command) {
        auto result = command->Execute();
        if (result.IsSuccess()) {
            history.push(std::move(command));
        }
        return result;
    }
    
    Result<void> Undo() {
        if (history.empty()) {
            return Result<void>::Failure(Error("No command to undo"));
        }
        auto result = history.top()->Undo();
        history.pop();
        return result;
    }
};
```

### Chain of Responsibility

```cpp
class IValidator {
public:
    virtual ~IValidator() = default;
    virtual Result<void> Validate(const InstallOptions& options) = 0;
    void SetNext(std::unique_ptr<IValidator> next) {
        nextValidator = std::move(next);
    }
    
protected:
    std::unique_ptr<IValidator> nextValidator;
};

class PathValidator : public IValidator {
    Result<void> Validate(const InstallOptions& options) override {
        if (options.ImagePath.empty()) {
            return Result<void>::Failure(Error("Image path is empty"));
        }
        return nextValidator ? nextValidator->Validate(options) 
                             : Result<void>::Success();
    }
};

class DiskSpaceValidator : public IValidator {
    Result<void> Validate(const InstallOptions& options) override {
        if (options.TargetDisk.size < options.RequiredSpace) {
            return Result<void>::Failure(Error("Insufficient disk space"));
        }
        return nextValidator ? nextValidator->Validate(options) 
                             : Result<void>::Success();
    }
};

auto validator = std::make_unique<PathValidator>();
validator->SetNext(std::make_unique<DiskSpaceValidator>());
auto result = validator->Validate(options);
```

## 함수형 패턴 (Functional Patterns)

### Monad Pattern

```cpp
template<typename T>
class Optional {
    std::optional<T> value;
    
public:
    template<typename F>
    auto Map(F&& func) -> Optional<decltype(func(std::declval<T>()))> {
        if (value.has_value()) {
            return Optional(func(*value));
        }
        return Optional();
    }
    
    template<typename F>
    auto FlatMap(F&& func) -> decltype(func(std::declval<T>())) {
        if (value.has_value()) {
            return func(*value);
        }
        return decltype(func(std::declval<T>()))();
    }
};

auto result = GetUser()
    .Map([](const User& u) { return u.GetEmail(); })
    .Map([](const Email& e) { return e.GetDomain(); });
```

### Pipeline Pattern

```cpp
auto result = disks
    | Filter([](const DiskInfo& d) { return d.busType != BusType::USB; })
    | Map([](const DiskInfo& d) { return d.size; })
    | Sort()
    | Take(5);
```

## 정리

이러한 패턴들을 적절히 조합하여 사용하면 유지보수 가능하고 확장 가능한 시스템을 구축할 수 있습니다.
