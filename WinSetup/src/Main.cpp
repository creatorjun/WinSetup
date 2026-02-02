#include <Windows.h>
#include <iostream>
#include <memory>
#include <string>

#include "domain/primitives/Expected.h"
#include "domain/primitives/Result.h"
#include "domain/primitives/Error.h"
#include "domain/functional/Optional.h"
#include "domain/memory/SmartPtr.h"
#include "domain/memory/UniqueResource.h"
#include "domain/memory/SharedResource.h"
#include "domain/validation/PathValidator.h"
#include "domain/validation/ValidationRules.h"

using namespace winsetup::domain;

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";

    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    return result;
}

void TestExpected() {
    std::wcout << L"=== Testing Expected<T, E> ===" << std::endl;

    auto success = Expected<int>::Success(42);
    if (success.HasValue()) {
        std::wcout << L"Success value: " << success.Value() << std::endl;
    }

    auto failure = Expected<int>::Failure(Error("Test error", 404));
    if (failure.HasError()) {
        std::wcout << L"Error message: " << Utf8ToWide(failure.GetError().Message()) << std::endl;
        std::wcout << L"Error code: " << failure.GetError().Code() << std::endl;
    }

    auto mapped = success.Map([](int val) { return val * 2; });
    std::wcout << L"Mapped value: " << mapped.Value() << std::endl;

    auto flatMapped = success.FlatMap([](int val) {
        if (val > 40) {
            return Expected<std::string>::Success("Large number");
        }
        return Expected<std::string>::Failure(Error("Small number"));
        });
    std::wcout << L"FlatMapped: " << Utf8ToWide(flatMapped.Value()) << std::endl;

    std::wcout << std::endl;
}

void TestResult() {
    std::wcout << L"=== Testing Result<E> ===" << std::endl;

    auto success = Result<>::Success();
    std::wcout << L"Result is success: " << (success.IsSuccess() ? L"Yes" : L"No") << std::endl;

    auto failure = Result<>::Failure(Error("Operation failed", 500));
    failure.OnError([](const Error& err) {
        std::wcout << L"Caught error: " << Utf8ToWide(err.Message()) << std::endl;
        });

    auto chained = success.Then([]() {
        std::wcout << L"First operation succeeded" << std::endl;
        return Result<>::Success();
        }).Then([]() {
            std::wcout << L"Second operation succeeded" << std::endl;
            return Result<>::Success();
            });

        std::wcout << std::endl;
}

void TestOptional() {
    std::wcout << L"=== Testing Optional<T> ===" << std::endl;

    Optional<int> opt1(100);
    std::wcout << L"Optional has value: " << (opt1.HasValue() ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Value: " << opt1.Value() << std::endl;

    Optional<int> opt2;
    std::wcout << L"Empty optional value or default: " << opt2.ValueOr(999) << std::endl;

    auto mapped = opt1.Map([](int val) { return val * 3; });
    std::wcout << L"Mapped value: " << mapped.Value() << std::endl;

    auto filtered = opt1.Filter([](int val) { return val > 50; });
    std::wcout << L"Filtered (>50): " << (filtered.HasValue() ? L"Passed" : L"Filtered out") << std::endl;

    std::wcout << std::endl;
}

void TestSmartPtr() {
    std::wcout << L"=== Testing SmartPtr<T> ===" << std::endl;

    {
        auto ptr = MakeSmartPtr<int>(777);
        std::wcout << L"SmartPtr value: " << *ptr << std::endl;
        std::wcout << L"SmartPtr is valid: " << (ptr ? L"Yes" : L"No") << std::endl;
    }
    std::wcout << L"SmartPtr destroyed (no memory leak)" << std::endl;

    {
        auto arrayPtr = MakeSmartPtr<int[]>(5);
        for (int i = 0; i < 5; ++i) {
            arrayPtr[i] = i * 10;
        }
        std::wcout << L"Array[2]: " << arrayPtr[2] << std::endl;
    }
    std::wcout << L"Array SmartPtr destroyed" << std::endl;

    std::wcout << std::endl;
}

void TestUniqueResource() {
    std::wcout << L"=== Testing UniqueResource<T> ===" << std::endl;

    {
        HANDLE fakeHandle = reinterpret_cast<HANDLE>(0x1234);
        auto resource = MakeUniqueResource(
            fakeHandle,
            [](HANDLE h) {
                std::wcout << L"Releasing handle: 0x" << std::hex << reinterpret_cast<uintptr_t>(h) << std::dec << std::endl;
            }
        );

        std::wcout << L"Resource handle: 0x" << std::hex << reinterpret_cast<uintptr_t>(resource.Get()) << std::dec << std::endl;
    }
    std::wcout << L"UniqueResource destroyed" << std::endl;

    std::wcout << std::endl;
}

void TestPathValidator() {
    std::wcout << L"=== Testing PathValidator ===" << std::endl;

    PathValidator validator;
    validator.AddRule(std::make_unique<InvalidCharacterRule>());
    validator.AddRule(std::make_unique<MaxLengthRule>(260));
    validator.AddRule(std::make_unique<AbsolutePathRule>());
    validator.AddRule(std::make_unique<ReservedNameRule>());

    std::vector<std::wstring> testPaths = {
        L"C:\\Windows\\System32",
        L"C:\\Invalid<Path>",
        L"RelativePath",
        L"C:\\CON\\file.txt",
        L"C:\\Valid\\Path\\To\\File.txt"
    };

    for (const auto& path : testPaths) {
        auto result = validator.Validate(path);
        std::wcout << L"Path: " << path << std::endl;
        std::wcout << L"Valid: " << (result.IsValid() ? L"Yes" : L"No") << std::endl;

        if (result.HasErrors()) {
            std::wcout << L"Errors:" << std::endl;
            for (const auto& error : result.GetErrors()) {
                std::wcout << L"  - " << error << std::endl;
            }
        }
        std::wcout << std::endl;
    }
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    std::wcout.imbue(std::locale(""));

    std::wcout << L"╔════════════════════════════════════════════╗" << std::endl;
    std::wcout << L"║   WinSetup - Domain Layer Test Suite      ║" << std::endl;
    std::wcout << L"╚════════════════════════════════════════════╝" << std::endl;
    std::wcout << std::endl;

    try {
        TestExpected();
        TestResult();
        TestOptional();
        TestSmartPtr();
        TestUniqueResource();
        TestPathValidator();

        std::wcout << L"╔════════════════════════════════════════════╗" << std::endl;
        std::wcout << L"║   All Tests Completed Successfully!       ║" << std::endl;
        std::wcout << L"╚════════════════════════════════════════════╝" << std::endl;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception: " << e.what() << std::endl;
    }

    std::wcout << std::endl << L"Press any key to exit..." << std::endl;
    std::cin.get();

    if (fp) {
        fclose(fp);
    }
    FreeConsole();

    return 0;
}
