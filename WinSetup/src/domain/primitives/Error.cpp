// src/domain/primitives/Error.cpp
#include "Error.h"
#include <sstream>

namespace winsetup::domain {

    std::wstring Error::ToString() const {
        std::wstringstream ss;
        ss << L"[Error] ";
        ss << m_message;
        ss << L" (Code: " << m_code << L")";
        return ss.str();
    }

}
