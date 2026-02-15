// src/abstractions/infrastructure/messaging/IEvent.h
#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <typeindex>

namespace winsetup::abstractions {

    class IEvent {
    public:
        virtual ~IEvent() = default;

        [[nodiscard]] virtual std::wstring GetEventType() const noexcept = 0;
        [[nodiscard]] virtual std::chrono::system_clock::time_point GetTimestamp() const noexcept = 0;
        [[nodiscard]] virtual std::type_index GetTypeIndex() const noexcept = 0;

        [[nodiscard]] virtual std::wstring ToString() const = 0;
        [[nodiscard]] virtual std::unique_ptr<IEvent> Clone() const = 0;
    };

    template<typename TDerived>
    class EventBase : public IEvent {
    public:
        EventBase()
            : mTimestamp(std::chrono::system_clock::now())
        {
        }

        [[nodiscard]] std::chrono::system_clock::time_point GetTimestamp() const noexcept override {
            return mTimestamp;
        }

        [[nodiscard]] std::type_index GetTypeIndex() const noexcept override {
            return std::type_index(typeid(TDerived));
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return TDerived::StaticEventType();
        }

    protected:
        std::chrono::system_clock::time_point mTimestamp;
    };

}
