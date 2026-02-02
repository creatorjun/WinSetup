#pragma once

#include <string>
#include <chrono>

namespace winsetup::application {

    struct ProgressEvent {
        std::string taskName;
        int currentStep;
        int totalSteps;
        double percentage;
        std::string statusMessage;
        std::chrono::milliseconds elapsedTime;
        std::chrono::milliseconds estimatedTimeRemaining;

        ProgressEvent()
            : currentStep(0)
            , totalSteps(0)
            , percentage(0.0)
            , elapsedTime(0)
            , estimatedTimeRemaining(0) {
        }

        ProgressEvent(
            std::string task,
            int current,
            int total,
            std::string message = ""
        ) : taskName(std::move(task))
            , currentStep(current)
            , totalSteps(total)
            , percentage(total > 0 ? (current * 100.0 / total) : 0.0)
            , statusMessage(std::move(message))
            , elapsedTime(0)
            , estimatedTimeRemaining(0) {
        }

        bool IsComplete() const noexcept {
            return currentStep >= totalSteps;
        }
    };

}
