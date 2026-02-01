<!-- docs/guides/testing.md -->

# 테스트 전략 (Testing Strategy)

## 개요

WinSetup 프로젝트는 Google Test 프레임워크를 사용하여 계층별 단위 테스트, 통합 테스트, 성능 테스트를 수행합니다.

## 테스트 피라미드

```
           ┌─────────────┐
           │  E2E Tests  │ (적음)
           └─────────────┘
         ┌───────────────────┐
         │ Integration Tests │ (중간)
         └───────────────────┘
    ┌──────────────────────────────┐
    │      Unit Tests              │ (많음)
    └──────────────────────────────┘
```

### 비율 목표
- **Unit Tests**: 70%
- **Integration Tests**: 20%
- **E2E Tests**: 10%

## 단위 테스트 (Unit Tests)

### Domain Layer 테스트

#### Expected 테스트

```cpp
TEST(ExpectedTest, HasValue_ReturnsTrue_WhenValuePresent) {
    Expected<int, Error> result = 42;
    
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(42, result.Value());
}

TEST(ExpectedTest, HasValue_ReturnsFalse_WhenError) {
    Expected<int, Error> result = Error("Something went wrong");
    
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ("Something went wrong", result.Error().Message());
}

TEST(ExpectedTest, Map_TransformsValue) {
    Expected<int, Error> result = 42;
    
    auto transformed = result.Map([](int x) { return x * 2; });
    
    ASSERT_TRUE(transformed.HasValue());
    EXPECT_EQ(84, transformed.Value());
}
```

#### PathValidator 테스트

```cpp
TEST(PathValidatorTest, ValidateWindowsPath_Success) {
    PathValidator validator;
    
    auto result = validator.Validate(L"C:\\Windows\\System32");
    
    ASSERT_TRUE(result.IsValid());
}

TEST(PathValidatorTest, ValidateInvalidPath_Failure) {
    PathValidator validator;
    
    auto result = validator.Validate(L"C:\\Invalid<>Path");
    
    ASSERT_FALSE(result.IsValid());
    EXPECT_FALSE(result.Errors().empty());
}
```

### Application Layer 테스트

#### EventBus 테스트

```cpp
TEST(EventBusTest, Publish_NotifiesSubscribers) {
    EventBus eventBus;
    int callCount = 0;
    
    eventBus.Subscribe<ProgressEvent>([&](const ProgressEvent& e) {
        callCount++;
    });
    
    eventBus.Publish(ProgressEvent(L"Test"));
    
    EXPECT_EQ(1, callCount);
}

TEST(EventBusTest, MultipleSubscribers_AllNotified) {
    EventBus eventBus;
    int count1 = 0, count2 = 0;
    
    eventBus.Subscribe<ProgressEvent>([&](const ProgressEvent&) { count1++; });
    eventBus.Subscribe<ProgressEvent>([&](const ProgressEvent&) { count2++; });
    
    eventBus.Publish(ProgressEvent(L"Test"));
    
    EXPECT_EQ(1, count1);
    EXPECT_EQ(1, count2);
}
```

#### TaskScheduler 테스트

```cpp
TEST(TaskSchedulerTest, Schedule_ExecutesTask) {
    MockExecutor mockExecutor;
    MockDispatcher mockDispatcher;
    TaskScheduler scheduler(mockExecutor, mockDispatcher);
    
    bool executed = false;
    auto task = scheduler.Schedule([&]{ executed = true; });
    
    task.Wait();
    
    EXPECT_TRUE(executed);
}
```

### Adapters Layer 테스트

#### Win32FileSystem 테스트

```cpp
TEST(Win32FileSystemTest, CreateDirectory_Success) {
    Win32FileSystem fileSystem;
    auto tempPath = std::filesystem::temp_directory_path() / L"test_dir";
    
    auto result = fileSystem.CreateDirectory(tempPath.wstring());
    
    ASSERT_TRUE(result.IsSuccess());
    EXPECT_TRUE(std::filesystem::exists(tempPath));
    
    std::filesystem::remove(tempPath);
}

TEST(Win32FileSystemTest, ListFiles_ReturnsFiles) {
    Win32FileSystem fileSystem;
    auto tempPath = std::filesystem::temp_directory_path();
    
    auto result = fileSystem.ListFiles(tempPath.wstring());
    
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value().empty());
}
```

## Mock 객체 사용

### Mock 인터페이스 정의

```cpp
class MockFileSystem : public IFileSystem {
    MOCK_METHOD(Result<void>, CreateDirectory, (const std::wstring& path), (override));
    MOCK_METHOD(Expected<std::vector<std::wstring>, Error>, ListFiles, 
                (const std::wstring& path), (override));
    MOCK_METHOD(Result<void>, DeleteFile, (const std::wstring& path), (override));
};

class MockEventBus : public IEventBus {
    std::vector<std::any> publishedEvents;
    
public:
    template<typename TEvent>
    void Subscribe(std::function<void(const TEvent&)>) override {}
    
    template<typename TEvent>
    void Publish(const TEvent& event) override {
        publishedEvents.push_back(event);
    }
    
    template<typename TEvent>
    std::vector<TEvent> GetPublishedEvents() const {
        std::vector<TEvent> events;
        for (const auto& e : publishedEvents) {
            if (e.type() == typeid(TEvent)) {
                events.push_back(std::any_cast<TEvent>(e));
            }
        }
        return events;
    }
};
```

### Mock 사용 예제

```cpp
TEST(DiskAnalyzerTest, AnalyzeDisk_PublishesProgressEvents) {
    MockFileSystem mockFs;
    MockEventBus mockEventBus;
    DiskAnalyzer analyzer(mockFs, mockEventBus);
    
    EXPECT_CALL(mockFs, ListFiles(_))
        .WillOnce(Return(std::vector<std::wstring>{L"file1.txt", L"file2.txt"}));
    
    analyzer.AnalyzeDisk(L"C:\\");
    
    auto events = mockEventBus.GetPublishedEvents<ProgressEvent>();
    EXPECT_FALSE(events.empty());
}
```

## 통합 테스트 (Integration Tests)

### 계층 간 통합 테스트

```cpp
TEST(IntegrationTest, InstallFlow_EndToEnd) {
    Win32Factory factory;
    auto fileSystem = factory.CreateFileSystem();
    auto threadPool = factory.CreateThreadPool(4);
    auto eventBus = std::make_unique<EventBus>();
    
    DependencyContainer container;
    container.Register<IFileSystem>(std::move(fileSystem));
    container.Register<IThreadPool>(std::move(threadPool));
    container.Register<IEventBus>(std::move(eventBus));
    
    auto orchestrator = container.Resolve<InstallOrchestrator>();
    
    InstallOptions options;
    options.ImagePath = L"C:\\test.wim";
    options.TargetDisk = DiskInfo{0, BusType::NVMe, 512ULL * 1024 * 1024 * 1024};
    
    auto result = orchestrator->Install(options);
    
    EXPECT_TRUE(result.IsSuccess());
}
```

### 파일 시스템 통합 테스트

```cpp
TEST(FileSystemIntegrationTest, CreateAndReadFile) {
    Win32FileSystem fileSystem;
    auto tempPath = std::filesystem::temp_directory_path() / L"test.txt";
    
    auto writeResult = fileSystem.WriteFile(tempPath.wstring(), L"Hello, World!");
    ASSERT_TRUE(writeResult.IsSuccess());
    
    auto readResult = fileSystem.ReadFile(tempPath.wstring());
    ASSERT_TRUE(readResult.HasValue());
    EXPECT_EQ(L"Hello, World!", readResult.Value());
    
    std::filesystem::remove(tempPath);
}
```

## 성능 테스트 (Performance Tests)

### 벤치마크 테스트

```cpp
TEST(PerformanceTest, DiskEnumeration_CompletesInUnder1Second) {
    Win32DiskManager diskManager;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto disks = diskManager.EnumerateDisks();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000);
}

TEST(PerformanceTest, TaskScheduler_Handles1000Tasks) {
    Win32ThreadPool threadPool(8);
    TaskScheduler scheduler(threadPool);
    
    std::atomic<int> counter = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        scheduler.Schedule([&]{ counter++; });
    }
    
    scheduler.WaitAll();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(1000, counter.load());
    EXPECT_LT(duration.count(), 5000);
}
```

### 메모리 누수 테스트

```cpp
TEST(MemoryTest, UniqueResource_NoLeak) {
    size_t initialMemory = GetCurrentMemoryUsage();
    
    {
        UniqueResource<HANDLE> handle(
            CreateFileW(L"test.txt", GENERIC_READ, 0, nullptr, 
                       OPEN_EXISTING, 0, nullptr),
            &CloseHandle
        );
    }
    
    size_t finalMemory = GetCurrentMemoryUsage();
    
    EXPECT_LE(finalMemory, initialMemory + 1024);
}
```

## 테스트 실행

### 개별 테스트 실행

```cmd
Tests.exe --gtest_filter=EventBusTest.*
```

### 모든 테스트 실행

```cmd
Tests.exe
```

### 테스트 결과 리포트

```cmd
Tests.exe --gtest_output=xml:test_results.xml
```

## 테스트 커버리지

### 목표
- **Domain Layer**: 90% 이상
- **Application Layer**: 85% 이상
- **Adapters Layer**: 75% 이상
- **Infrastructure Layer**: 60% 이상

### 커버리지 측정

```cmd
scripts\test.bat --coverage
```

## 지속적 통합 (CI)

### GitHub Actions 예제

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: msbuild WinSetup.sln /p:Configuration=Release
      - name: Run Tests
        run: Tests.exe --gtest_output=xml:test_results.xml
      - name: Upload Results
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: test_results.xml
```

## 테스트 작성 가이드라인

### 명명 규칙

```
MethodName_StateUnderTest_ExpectedBehavior
```

예제:
- `CreateDirectory_WithValidPath_ReturnsSuccess`
- `Publish_WithNoSubscribers_DoesNotThrow`
- `Map_WithNullValue_ReturnsNullOptional`

### AAA 패턴

```cpp
TEST(ExampleTest, TestName) {
    MockFileSystem mockFs;
    DiskAnalyzer analyzer(mockFs);
    
    auto result = analyzer.Analyze();
    
    ASSERT_TRUE(result.IsSuccess());
    EXPECT_EQ(expected, result.Value());
}
```

## 정리

철저한 테스트를 통해 안정적이고 신뢰할 수 있는 시스템을 구축합니다.
