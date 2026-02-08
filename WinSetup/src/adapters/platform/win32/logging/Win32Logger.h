// src/adapters/platform/win32/logging/Win32Logger.h

#pragma once

#include "../../../../abstractions/infrastructure/logging/ILogger.h"
#include <fstream>
#include <mutex>
#include <string>

namespace winsetup::adapters::platform {

    class Win32Logger : public abstractions::ILogger {
    public:
        explicit Win32Logger(const std::wstring& logFilePath = L"WinSetup.log");
        ~Win32Logger() override;

        void Log(
            abstractions::LogLevel level,
            const std::wstring& message,
            const std::source_location& location
        ) override;

    private:
        std::wstring GetLevelString(abstractions::LogLevel level) const;
        std::wstring GetTimestamp() const;

        std::wofstream m_logFile;
        std::mutex m_mutex;
        std::wstring m_logFilePath;
    };

}
