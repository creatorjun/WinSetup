<!-- docs/api/domain.md -->

# Domain API Reference

## 개요

Layer 1 Domain은 핵심 비즈니스 로직과 엔티티를 정의합니다. 외부 의존성이 전혀 없는 순수 C++ 코드로 구성됩니다.

## Primitives

### Expected<T, E>

값 또는 에러를 포함하는 모나드 타입입니다.

```cpp
template<typename T, typename E = Error>
class Expected {
public:
    static Expected Success(T value);
    static Expected Failure(E error);
    
    bool HasValue() const;
    const T& Value() const;
    T& Value();
    const E& Error() const;
    
    template<typename F>
    auto Map(F&& func) const -> Expected<decltype(func(std::declval<T>())), E>;
    
    template<typename F>
    auto FlatMap(F&& func) const -> decltype(func(std::declval<T>()));
    
    T ValueOr(T defaultValue) const;
};
```

#### 사용 예제

```cpp
Expected<int, Error> Divide(int a, int b) {
    if (b == 0) {
        return Expected<int, Error>::Failure(Error("Division by zero"));
    }
    return Expected<int, Error>::Success(a / b);
}

auto result = Divide(10, 2)
    .Map([](int x) { return x * 2; })
    .Map([](int x) { return std::to_string(x); });

if (result.HasValue()) {
    std::cout << "Result: " << result.Value() << std::endl;
} else {
    std::cerr << "Error: " << result.Error().Message() << std::endl;
}
```

### Result<E>

성공/실패만 나타내는 타입입니다.

```cpp
template<typename E = Error>
class Result {
public:
    static Result Success();
    static Result Failure(E error);
    
    bool IsSuccess() const;
    bool IsFailure() const;
    const E& Error() const;
    
    template<typename F>
    Result Then(F&& func) const;
};
```

#### 사용 예제

```cpp
Result<Error> ValidatePath(const std::wstring& path) {
    if (path.empty()) {
        return Result<Error>::Failure(Error("Path is empty"));
    }
    if (path.find(L'<') != std::wstring::npos) {
        return Result<Error>::Failure(Error("Invalid character in path"));
    }
    return Result<Error>::Success();
}

auto result = ValidatePath(L"C:\\Program Files");
if (result.IsSuccess()) {
    std::cout << "Path is valid" << std::endl;
}
```

### Error

에러 정보를 담는 타입입니다.

```cpp
class Error {
public:
    explicit Error(std::string message);
    Error(std::string message, int code);
    
    const std::string& Message() const;
    int Code() const;
    bool HasCode() const;
    
    static Error FromWin32(DWORD errorCode);
    static Error FromHRESULT(HRESULT hr);
    static Error FromErrno(int err);
};
```

#### 사용 예제

```cpp
Result<void> OpenFile(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, 
                              OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return Result<void>::Failure(Error::FromWin32(GetLastError()));
    }
    CloseHandle(hFile);
    return Result<void>::Success();
}
```

## Functional

### Optional<T>

값이 있을 수도 없을 수도 있는 타입입니다.

```cpp
template<typename T>
class Optional {
public:
    Optional();
    Optional(T value);
    Optional(std::nullopt_t);
    
    bool HasValue() const;
    const T& Value() const;
    T& Value();
    T ValueOr(T defaultValue) const;
    
    template<typename F>
    auto Map(F&& func) const -> Optional<decltype(func(std::declval<T>()))>;
    
    template<typename F>
    auto FlatMap(F&& func) const -> decltype(func(std::declval<T>()));
    
    template<typename F>
    Optional Filter(F&& predicate) const;
};
```

#### 사용 예제

```cpp
struct User {
    int id;
    std::string name;
};

Optional<User> FindUser(int id) {
    if (id == 1) {
        return User{1, "John"};
    }
    return std::nullopt;
}

auto userName = FindUser(1)
    .Map([](const User& u) { return u.name; })
    .ValueOr("Unknown");
```

### Monads

함수형 프로그래밍 모나드를 제공합니다.

```cpp
namespace Monads {
    template<typename T, typename F>
    auto Map(const std::vector<T>& container, F&& func) 
        -> std::vector<decltype(func(std::declval<T>()))>;
    
    template<typename T, typename F>
    auto Filter(const std::vector<T>& container, F&& predicate) 
        -> std::vector<T>;
    
    template<typename T, typename U, typename F>
    auto Reduce(const std::vector<T>& container, U init, F&& func) -> U;
    
    template<typename T, typename F>
    auto FlatMap(const std::vector<T>& container, F&& func)
        -> std::vector<typename decltype(func(std::declval<T>()))::value_type>;
}
```

#### 사용 예제

```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};

auto squares = Monads::Map(numbers, [](int x) { return x * x; });

auto evens = Monads::Filter(numbers, [](int x) { return x % 2 == 0; });

auto sum = Monads::Reduce(numbers, 0, [](int acc, int x) { return acc + x; });
```

### Pipeline

함수형 파이프라인 연산을 제공합니다.

```cpp
template<typename T>
class Pipeline {
public:
    explicit Pipeline(std::vector<T> data);
    
    template<typename F>
    Pipeline<T> Filter(F&& predicate) const;
    
    template<typename F>
    auto Map(F&& func) const -> Pipeline<decltype(func(std::declval<T>()))>;
    
    Pipeline<T> Sort() const;
    Pipeline<T> Sort(std::function<bool(const T&, const T&)> comparator) const;
    
    Pipeline<T> Take(size_t count) const;
    Pipeline<T> Skip(size_t count) const;
    Pipeline<T> Reverse() const;
    Pipeline<T> Distinct() const;
    
    std::vector<T> ToVector() const;
    size_t Count() const;
};
```

#### 사용 예제

```cpp
struct DiskInfo {
    uint32_t index;
    BusType busType;
    uint64_t size;
};

std::vector<DiskInfo> disks = GetAllDisks();

auto result = Pipeline(disks)
    .Filter([](const DiskInfo& d) { return d.busType != BusType::USB; })
    .Sort([](const DiskInfo& a, const DiskInfo& b) { 
        return a.size > b.size; 
    })
    .Take(3)
    .ToVector();
```

### Compose

함수 합성을 제공합니다.

```cpp
template<typename F, typename G>
auto Compose(F&& f, G&& g) {
    return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&& x) {
        return f(g(std::forward<decltype(x)>(x)));
    };
}

template<typename... Funcs>
auto Pipe(Funcs&&... funcs) {
    return [funcs...](auto&& x) {
        return (funcs(std::forward<decltype(x)>(x)), ...);
    };
}
```

#### 사용 예제

```cpp
auto addOne = [](int x) { return x + 1; };
auto multiplyTwo = [](int x) { return x * 2; };
auto toString = [](int x) { return std::to_string(x); };

auto composed = Compose(toString, Compose(multiplyTwo, addOne));
auto result = composed(5);
```

## Memory

### SmartPtr<T>

커스텀 스마트 포인터입니다.

```cpp
template<typename T>
class SmartPtr {
public:
    explicit SmartPtr(T* ptr = nullptr);
    ~SmartPtr();
    
    SmartPtr(const SmartPtr&) = delete;
    SmartPtr& operator=(const SmartPtr&) = delete;
    
    SmartPtr(SmartPtr&& other) noexcept;
    SmartPtr& operator=(SmartPtr&& other) noexcept;
    
    T* Get() const;
    T* Release();
    void Reset(T* ptr = nullptr);
    
    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const;
};
```

### UniqueResource<T>

RAII 방식 리소스 관리를 제공합니다.

```cpp
template<typename T, typename Deleter = std::function<void(T)>>
class UniqueResource {
public:
    UniqueResource(T resource, Deleter deleter);
    ~UniqueResource();
    
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator=(const UniqueResource&) = delete;
    
    UniqueResource(UniqueResource&& other) noexcept;
    UniqueResource& operator=(UniqueResource&& other) noexcept;
    
    T Get() const;
    T Release();
    void Reset(T newResource);
};
```

#### 사용 예제

```cpp
auto CloseHandleDeleter = [](HANDLE h) {
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
};

UniqueResource<HANDLE, decltype(CloseHandleDeleter)> handle(
    CreateFileW(L"test.txt", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr),
    CloseHandleDeleter
);

if (handle.Get() != INVALID_HANDLE_VALUE) {
    DWORD bytesRead;
    char buffer;
    ReadFile(handle.Get(), buffer, sizeof(buffer), &bytesRead, nullptr);
}
```

### SharedResource<T>

참조 카운팅 기반 공유 리소스 관리를 제공합니다.

```cpp
template<typename T, typename Deleter = std::function<void(T)>>
class SharedResource {
public:
    SharedResource(T resource, Deleter deleter);
    ~SharedResource();
    
    SharedResource(const SharedResource& other);
    SharedResource& operator=(const SharedResource& other);
    
    SharedResource(SharedResource&& other) noexcept;
    SharedResource& operator=(SharedResource&& other) noexcept;
    
    T Get() const;
    size_t UseCount() const;
    bool IsUnique() const;
};
```

#### 사용 예제

```cpp
auto deleter = [](FILE* f) { if (f) fclose(f); };

SharedResource<FILE*, decltype(deleter)> file(
    fopen("data.txt", "r"),
    deleter
);

SharedResource<FILE*, decltype(deleter)> fileCopy = file;

std::cout << "Use count: " << file.UseCount() << std::endl;
```

### PoolAllocator<T>

오브젝트 풀 기반 메모리 할당자입니다.

```cpp
template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(size_t poolSize);
    ~PoolAllocator();
    
    T* Allocate();
    void Deallocate(T* ptr);
    
    size_t GetPoolSize() const;
    size_t GetAvailableCount() const;
    size_t GetAllocatedCount() const;
    
    void Clear();
};
```

#### 사용 예제

```cpp
struct Task {
    std::function<void()> work;
    void Execute() { if (work) work(); }
};

PoolAllocator<Task> taskPool(1000);

for (int i = 0; i < 100; ++i) {
    Task* task = taskPool.Allocate();
    new (task) Task{[i]{ std::cout << "Task " << i << std::endl; }};
    
    task->Execute();
    
    task->~Task();
    taskPool.Deallocate(task);
}

std::cout << "Available: " << taskPool.GetAvailableCount() << std::endl;
std::cout << "Allocated: " << taskPool.GetAllocatedCount() << std::endl;
```

## Validation

### PathValidator

경로 유효성 검증을 제공합니다.

```cpp
class PathValidator {
public:
    PathValidator();
    
    ValidationResult Validate(const std::wstring& path) const;
    
    void AddRule(std::unique_ptr<IValidationRule> rule);
    void RemoveRule(const std::string& ruleName);
    void ClearRules();
};
```

#### 사용 예제

```cpp
PathValidator validator;
validator.AddRule(std::make_unique<InvalidCharacterRule>());
validator.AddRule(std::make_unique<MaxLengthRule>(260));
validator.AddRule(std::make_unique<AbsolutePathRule>());

auto result = validator.Validate(L"C:\\Program Files\\MyApp");

if (result.IsValid()) {
    std::wcout << L"Path is valid" << std::endl;
} else {
    for (const auto& error : result.Errors()) {
        std::wcerr << L"Error: " << error << std::endl;
    }
}
```

### ValidationRules

표준 검증 규칙들입니다.

```cpp
class IValidationRule {
public:
    virtual ~IValidationRule() = default;
    virtual std::string GetName() const = 0;
    virtual bool Validate(const std::wstring& path) const = 0;
    virtual std::wstring GetErrorMessage() const = 0;
};

class InvalidCharacterRule : public IValidationRule {
public:
    std::string GetName() const override;
    bool Validate(const std::wstring& path) const override;
    std::wstring GetErrorMessage() const override;
};

class MaxLengthRule : public IValidationRule {
    size_t maxLength_;
public:
    explicit MaxLengthRule(size_t maxLength);
    std::string GetName() const override;
    bool Validate(const std::wstring& path) const override;
    std::wstring GetErrorMessage() const override;
};

class AbsolutePathRule : public IValidationRule {
public:
    std::string GetName() const override;
    bool Validate(const std::wstring& path) const override;
    std::wstring GetErrorMessage() const override;
};

class ReservedNameRule : public IValidationRule {
public:
    std::string GetName() const override;
    bool Validate(const std::wstring& path) const override;
    std::wstring GetErrorMessage() const override;
};
```

### ValidationResult

검증 결과를 담는 타입입니다.

```cpp
class ValidationResult {
public:
    static ValidationResult Valid();
    static ValidationResult Invalid(std::vector<std::wstring> errors);
    
    bool IsValid() const;
    const std::vector<std::wstring>& Errors() const;
    
    void AddError(std::wstring error);
    void Merge(const ValidationResult& other);
};
```

#### 사용 예제

```cpp
ValidationResult ValidateInstallPath(const std::wstring& path) {
    ValidationResult result = ValidationResult::Valid();
    
    if (path.empty()) {
        result.AddError(L"Path cannot be empty");
    }
    
    if (path.length() > 260) {
        result.AddError(L"Path is too long");
    }
    
    if (path.find(L"<>:\"|?*") != std::wstring::npos) {
        result.AddError(L"Path contains invalid characters");
    }
    
    return result;
}
```

## 도메인 엔티티

### DiskInfo

디스크 정보를 담는 엔티티입니다.

```cpp
struct DiskInfo {
    uint32_t index;
    BusType busType;
    uint64_t size;
    std::wstring path;
    std::wstring model;
    bool isRemovable;
    
    bool operator==(const DiskInfo& other) const;
    bool operator<(const DiskInfo& other) const;
};

enum class BusType {
    Unknown = 0,
    SCSI = 1,
    ATAPI = 2,
    ATA = 3,
    IEEE1394 = 4,
    SSA = 5,
    FibreChannel = 6,
    USB = 7,
    RAID = 8,
    iSCSI = 9,
    SAS = 10,
    SATA = 11,
    SD = 12,
    MMC = 13,
    Virtual = 14,
    FileBackedVirtual = 15,
    NVMe = 17
};
```

### PartitionInfo

파티션 정보를 담는 엔티티입니다.

```cpp
struct PartitionInfo {
    uint32_t index;
    uint64_t offset;
    uint64_t size;
    std::wstring driveLetter;
    std::wstring fileSystem;
    PartitionType type;
    bool isBootable;
    
    bool operator==(const PartitionInfo& other) const;
};

enum class PartitionType {
    Unknown,
    System,
    Data,
    Recovery,
    EFI
};
```

### InstallOptions

설치 옵션을 담는 값 객체입니다.

```cpp
struct InstallOptions {
    std::wstring imagePath;
    DiskInfo targetDisk;
    bool preserveData;
    bool injectDrivers;
    std::wstring driverPath;
    uint64_t requiredSpace;
    
    ValidationResult Validate() const;
};
```

## 도메인 서비스

### DiskPriorityComparator

디스크 우선순위 비교자입니다.

```cpp
struct DiskPriorityComparator {
    bool operator()(const DiskInfo& a, const DiskInfo& b) const {
        if (a.busType != b.busType) {
            return GetBusPriority(a.busType) > GetBusPriority(b.busType);
        }
        return a.size < b.size;
    }
    
private:
    static int GetBusPriority(BusType type) {
        switch (type) {
            case BusType::NVMe: return 100;
            case BusType::SATA: return 90;
            case BusType::SAS: return 85;
            case BusType::SCSI: return 80;
            case BusType::USB: return 10;
            default: return 50;
        }
    }
};
```

#### 사용 예제

```cpp
std::vector<DiskInfo> disks = GetAllDisks();
std::sort(disks.begin(), disks.end(), DiskPriorityComparator{});

auto systemDisk = disks;
auto dataDisk = disks.size() > 1 ? disks : DiskInfo{};[1]
```

## 정리

Domain 계층은 순수한 비즈니스 로직만 포함하며, 외부 의존성 없이 테스트 가능합니다.
