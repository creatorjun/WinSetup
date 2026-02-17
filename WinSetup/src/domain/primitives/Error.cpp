// src/domain/primitives/Error.cpp
#include "Error.h"

namespace winsetup::domain {

    Error::Error()
        : mMessage(L"Unknown error")
        , mCode(0)
        , mCategory(ErrorCategory::Unknown)
    {
    }

    Error::Error(const std::wstring& message, uint32_t code, ErrorCategory category)
        : mMessage(message)
        , mCode(code)
        , mCategory(category)
    {
    }

    std::wstring Error::ToString() const {
        return L"[" + CategoryToString(mCategory) + L"] " + mMessage + L" (Code: " + std::to_wstring(mCode) + L")";
    }

    std::wstring Error::CategoryToString(ErrorCategory category) {
        switch (category) {
        case ErrorCategory::Unknown: return L"Unknown";
        case ErrorCategory::System: return L"System";
        case ErrorCategory::IO: return L"IO";
        case ErrorCategory::Parsing: return L"Parsing";
        case ErrorCategory::Validation: return L"Validation";
        case ErrorCategory::NotImplemented: return L"NotImplemented";
        case ErrorCategory::Disk: return L"Disk";
        case ErrorCategory::Volume: return L"Volume";
        case ErrorCategory::Partition: return L"Partition";
        case ErrorCategory::FileSystem: return L"FileSystem";
        case ErrorCategory::Imaging: return L"Imaging";
        case ErrorCategory::Driver: return L"Driver";
        case ErrorCategory::Network: return L"Network";
        case ErrorCategory::Registry: return L"Registry";
        default: return L"Unknown";
        }
    }

}
