// src/domain/valueobjects/DiskSize.cpp
#include "DiskSize.h"
#include <sstream>
#include <iomanip>

namespace winsetup::domain {

    std::wstring DiskSize::ToString() const {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(2);

        if (m_bytes >= TB) {
            oss << ToTB() << L" TB";
        }
        else if (m_bytes >= GB) {
            oss << ToGB() << L" GB";
        }
        else if (m_bytes >= MB) {
            oss << ToMB() << L" MB";
        }
        else if (m_bytes >= KB) {
            oss << ToKB() << L" KB";
        }
        else {
            oss << m_bytes << L" Bytes";
        }

        return oss.str();
    }

}
