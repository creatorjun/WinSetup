// src/domain/primitives/Error.cpp

#include "Error.h"
#include <sstream>
#include <iomanip>

namespace winsetup::domain {

    std::wstring Error::ToString() const {
        std::wostringstream oss;

        oss << L"[Error] " << m_message;

        if (m_code != 0) {
            oss << L" (Code: " << m_code << L")";
        }

        oss << L" [Category: ";
        switch (m_category) {
        case ErrorCategory::System:        oss << L"System"; break;
        case ErrorCategory::Disk:          oss << L"Disk"; break;
        case ErrorCategory::Volume:        oss << L"Volume"; break;
        case ErrorCategory::Imaging:       oss << L"Imaging"; break;
        case ErrorCategory::Configuration: oss << L"Configuration"; break;
        case ErrorCategory::Network:       oss << L"Network"; break;
        case ErrorCategory::Permission:    oss << L"Permission"; break;
        default:                           oss << L"Unknown"; break;
        }
        oss << L"]";

        if (!m_contexts.empty()) {
            oss << L"\nContext:\n";
            for (const auto& ctx : m_contexts) {
                oss << L"  - " << ctx.function << L" (" << ctx.file << L":" << ctx.line << L")\n";
            }
        }

        return oss.str();
    }

}
