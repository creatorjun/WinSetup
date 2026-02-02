// src/application/events/InstallEvent.h 
#pragma once

#include <string>
#include <chrono>

namespace winsetup::application {

    enum class InstallPhase {
        Initializing,
        Downloading,
        Extracting,
        Installing,
        Configuring,
        Finalizing,
        Completed,
        Failed,
        Cancelled
    };

    struct InstallEvent {
        InstallPhase phase;
        std::string packageName;
        std::string version;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        bool success;

        InstallEvent()
            : phase(InstallPhase::Initializing)
            , timestamp(std::chrono::system_clock::now())
            , success(true) {
        }

        InstallEvent(
            InstallPhase p,
            std::string pkg,
            std::string ver = "",
            std::string desc = ""
        ) : phase(p)
            , packageName(std::move(pkg))
            , version(std::move(ver))
            , description(std::move(desc))
            , timestamp(std::chrono::system_clock::now())
            , success(p != InstallPhase::Failed && p != InstallPhase::Cancelled) {
        }

        bool IsTerminal() const noexcept {
            return phase == InstallPhase::Completed ||
                phase == InstallPhase::Failed ||
                phase == InstallPhase::Cancelled;
        }
    };

}
