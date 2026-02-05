// src/abstractions/async/IScheduler.h 
#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <functional>

namespace winsetup::abstractions {

    using ScheduleId = size_t;

    enum class ScheduleMode {
        Once,
        Periodic,
        Delayed
    };

    struct ScheduleOptions {
        ScheduleMode mode{ ScheduleMode::Once };
        std::chrono::milliseconds initialDelay{ 0 };
        std::chrono::milliseconds interval{ 0 };
        int maxExecutions{ -1 };
        bool executeOnSchedulerThread{ false };

        static ScheduleOptions Once(std::chrono::milliseconds delay = std::chrono::milliseconds(0)) {
            ScheduleOptions opts;
            opts.mode = ScheduleMode::Once;
            opts.initialDelay = delay;
            return opts;
        }

        static ScheduleOptions Periodic(std::chrono::milliseconds intervalMs, int maxExec = -1) {
            ScheduleOptions opts;
            opts.mode = ScheduleMode::Periodic;
            opts.interval = intervalMs;
            opts.maxExecutions = maxExec;
            return opts;
        }

        static ScheduleOptions Delayed(std::chrono::milliseconds delay) {
            ScheduleOptions opts;
            opts.mode = ScheduleMode::Delayed;
            opts.initialDelay = delay;
            return opts;
        }
    };

    class IScheduler {
    public:
        virtual ~IScheduler() = default;

        virtual ScheduleId Schedule(
            std::function<void()> work,
            const ScheduleOptions& options
        ) = 0;

        virtual ScheduleId ScheduleAt(
            std::function<void()> work,
            std::chrono::system_clock::time_point timePoint
        ) = 0;

        virtual ScheduleId ScheduleAfter(
            std::function<void()> work,
            std::chrono::milliseconds delay
        ) = 0;

        virtual ScheduleId SchedulePeriodic(
            std::function<void()> work,
            std::chrono::milliseconds interval,
            std::chrono::milliseconds initialDelay = std::chrono::milliseconds(0)
        ) = 0;

        virtual bool Cancel(ScheduleId scheduleId) = 0;
        virtual void CancelAll() = 0;

        [[nodiscard]] virtual bool IsScheduled(ScheduleId scheduleId) const = 0;
        [[nodiscard]] virtual size_t GetScheduledCount() const = 0;

        [[nodiscard]] virtual std::chrono::system_clock::time_point
            GetNextExecutionTime(ScheduleId scheduleId) const = 0;

        virtual void Pause() = 0;
        virtual void Resume() = 0;
        [[nodiscard]] virtual bool IsPaused() const = 0;

        virtual void Shutdown() = 0;
    };

}
