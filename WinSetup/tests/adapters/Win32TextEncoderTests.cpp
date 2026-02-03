// tests/adapters/Win32TextEncoderTests.cpp 
#include "../../../src/adapters/platform/windows/encoding/Win32TextEncoder.h"
#include <cassert>
#include <iostream>

using namespace winsetup;

void TestBasicUtf8ToWide() {
    adapters::Win32TextEncoder encoder;

    auto result = encoder.ToWide("Hello, World!");
    assert(result.HasValue());
    assert(result.Value() == L"Hello, World!");
}

void TestBasicWideToUtf8() {
    adapters::Win32TextEncoder encoder;

    auto result = encoder.ToUtf8(L"Hello, World!");
    assert(result.HasValue());
    assert(result.Value() == "Hello, World!");
}

void TestKoreanCharacters() {
    adapters::Win32TextEncoder encoder;

    const std::string utf8Korean = "안녕하세요";
    auto wideResult = encoder.ToWide(utf8Korean);
    assert(wideResult.HasValue());

    auto utf8Result = encoder.ToUtf8(wideResult.Value());
    assert(utf8Result.HasValue());
    assert(utf8Result.Value() == utf8Korean);
}

void TestEmptyString() {
    adapters::Win32TextEncoder encoder;

    auto wideResult = encoder.ToWide("");
    assert(wideResult.HasValue());
    assert(wideResult.Value().empty());

    auto utf8Result = encoder.ToUtf8(L"");
    assert(utf8Result.HasValue());
    assert(utf8Result.Value().empty());
}

void TestInvalidUtf8() {
    adapters::Win32TextEncoder encoder;

    const std::string invalidUtf8 = "\xFF\xFE\xFD";
    auto result = encoder.ToWide(invalidUtf8);
    assert(result.HasError());
}

void TestUtf8Validation() {
    adapters::Win32TextEncoder encoder;

    assert(encoder.IsValidUtf8("Valid UTF-8"));
    assert(encoder.IsValidUtf8("안녕하세요"));
    assert(!encoder.IsValidUtf8("\xFF\xFE\xFD"));
}

void TestUtf16Validation() {
    adapters::Win32TextEncoder encoder;

    assert(encoder.IsValidUtf16(L"Valid UTF-16"));
    assert(encoder.IsValidUtf16(L"안녕하세요"));
}

void TestSpecialCharacters() {
    adapters::Win32TextEncoder encoder;

    const std::string special = "©®™€£¥";
    auto wideResult = encoder.ToWide(special);
    assert(wideResult.HasValue());

    auto utf8Result = encoder.ToUtf8(wideResult.Value());
    assert(utf8Result.HasValue());
    assert(utf8Result.Value() == special);
}

void TestLargeString() {
    adapters::Win32TextEncoder encoder;

    std::string large(10000, 'A');
    auto wideResult = encoder.ToWide(large);
    assert(wideResult.HasValue());
    assert(wideResult.Value().size() == 10000);

    auto utf8Result = encoder.ToUtf8(wideResult.Value());
    assert(utf8Result.HasValue());
    assert(utf8Result.Value() == large);
}

void TestUtf16Encoding() {
    adapters::Win32TextEncoder encoder;

    const std::wstring wideStr = L"Test";
    auto result = encoder.Encode(wideStr, abstractions::TextEncoding::UTF16LE);
    assert(result.HasValue());
    assert(result.Value().size() == wideStr.size() * 2);
}

int main() {
    try {
        TestBasicUtf8ToWide();
        TestBasicWideToUtf8();
        TestKoreanCharacters();
        TestEmptyString();
        TestInvalidUtf8();
        TestUtf8Validation();
        TestUtf16Validation();
        TestSpecialCharacters();
        TestLargeString();
        TestUtf16Encoding();

        std::cout << "All Win32TextEncoder tests passed!\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}
