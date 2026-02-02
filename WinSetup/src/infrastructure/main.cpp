// src/infrastructure/main.cpp

#include <Windows.h>
#include <iostream>
#include <memory>
#include "../domain/primitives/Error.h"
#include "../domain/primitives/Result.h"
#include "../domain/primitives/Expected.h"
#include "../domain/validation/PathValidator.h"
#include "../domain/validation/ValidationRules.h"
#include "../domain/validation/ValidationResult.h"

using namespace winsetup::domain;

Result<void> RunApplication(HINSTANCE hInstance, int nShowCmd);
Result<void> InitializeDomain();
Result<void> TestDomainPrimitives();

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AllocConsole();
    FILE* pConsole;
    freopen_s(&pConsole, "CONOUT$", "w", stdout);
    freopen_s(&pConsole, "CONOUT$", "w", stderr);

    std::wcout << L"==================================" << std::endl;
    std::wcout << L"  WinSetup - Clean Architecture  " << std::endl;
    std::wcout << L"==================================" << std::endl;
    std::wcout << std::endl;

    auto result = RunApplication(hInstance, nShowCmd);

    if (result.IsFailure()) {
        std::wcerr << L"Application failed: "
            << std::wstring(result.GetError().Message().begin(),
                result.GetError().Message().end())
            << std::endl;
        MessageBoxW(nullptr, L"Application failed to start", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::wcout << L"\nPress any key to exit..." << std::endl;
    std::cin.get();

    FreeConsole();
    return 0;
}

int main()
{
    return wWinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}

Result<void> RunApplication(HINSTANCE hInstance, int nShowCmd)
{
    std::wcout << L"[Phase 1] Initializing Domain Layer..." << std::endl;
    auto domainInit = InitializeDomain();
    if (domainInit.IsFailure()) {
        return domainInit;
    }

    std::wcout << L"[Phase 2] Testing Domain Primitives..." << std::endl;
    auto testResult = TestDomainPrimitives();
    if (testResult.IsFailure()) {
        return testResult;
    }

    std::wcout << L"[Phase 3] Application Ready" << std::endl;
    std::wcout << L"\nDomain layer initialized successfully!" << std::endl;

    return Result<void>::Success();
}

Result<void> InitializeDomain()
{
    std::wcout << L"  - Error handling initialized" << std::endl;
    std::wcout << L"  - Result<T> monad initialized" << std::endl;
    std::wcout << L"  - Expected<T,E> monad initialized" << std::endl;
    std::wcout << L"  - Validation system initialized" << std::endl;

    return Result<void>::Success();
}

Result<void> TestDomainPrimitives()
{
    std::wcout << L"  Testing Expected<T>..." << std::endl;

    auto divide = [](int a, int b) -> Expected<int, Error> {
        if (b == 0) {
            return Expected<int, Error>::Failure(Error("Division by zero"));
        }
        return Expected<int, Error>::Success(a / b);
        };

    auto result = divide(10, 2)
        .Map([](int x) { return x * 2; })
        .Map([](int x) { return x + 5; });

    if (result.HasValue()) {
        std::wcout << L"    Result: " << result.Value() << L" (Expected: 25)" << std::endl;
        if (result.Value() != 25) {
            return Result<void>::Failure(Error("Expected monad test failed"));
        }
    }

    std::wcout << L"  Testing Result<void>..." << std::endl;

    auto validatePath = [](const std::wstring& path) -> Result<void> {
        if (path.empty()) {
            return Result<void>::Failure(Error("Path is empty"));
        }
        if (path.find(L'<') != std::wstring::npos) {
            return Result<void>::Failure(Error("Invalid character in path"));
        }
        return Result<void>::Success();
        };

    auto pathResult = validatePath(L"C:\\Windows\\System32");
    if (pathResult.IsSuccess()) {
        std::wcout << L"    Path validation: PASS" << std::endl;
    }
    else {
        return Result<void>::Failure(Error("Path validation test failed"));
    }

    std::wcout << L"  Testing PathValidator..." << std::endl;

    PathValidator validator;
    validator.AddRule(std::make_unique<InvalidCharacterRule>());
    validator.AddRule(std::make_unique<MaxLengthRule>(260));
    validator.AddRule(std::make_unique<AbsolutePathRule>());

    auto validationResult = validator.Validate(L"C:\\Program Files\\WinSetup");
    if (validationResult.IsValid()) {
        std::wcout << L"    PathValidator: PASS" << std::endl;
    }
    else {
        std::wcout << L"    PathValidator: FAIL" << std::endl;
        for (const auto& error : validationResult.Errors()) {
            std::wcout << L"      - " << error << std::endl;
        }
        return Result<void>::Failure(Error("PathValidator test failed"));
    }

    std::wcout << L"  All tests passed!" << std::endl;
    return Result<void>::Success();
}
