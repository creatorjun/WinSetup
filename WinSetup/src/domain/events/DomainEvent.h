// src/domain/events/DomainEvent.h
#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <atomic>
#include <memory>
#include <typeindex>

namespace winsetup::domain {

    class DomainEvent {
    public:
        virtual ~DomainEvent() = default;

        [[nodiscard]] uint64_t     GetEventId()   const noexcept { return mEventId; }
        [[nodiscard]] auto         GetTimestamp() const noexcept { return mTimestamp; }
        [[nodiscard]] virtual std::wstring       GetEventType()  const noexcept = 0;
        [[nodiscard]] virtual std::type_index    GetTypeIndex()  const noexcept = 0;
        [[nodiscard]] virtual std::wstring       ToString()      const = 0;
        [[nodiscard]] virtual std::unique_ptr<DomainEvent> Clone() const = 0;

    protected:
        DomainEvent()
            : mEventId(GenerateEventId())
            , mTimestamp(std::chrono::system_clock::now())
        {
        }

    private:
        static uint64_t GenerateEventId() noexcept {
            static std::atomic<uint64_t> counter{ 0 };
            return counter.fetch_add(1, std::memory_order_relaxed);
        }

        uint64_t                               mEventId;
        std::chrono::system_clock::time_point  mTimestamp;
    };

    template<typename TDerived>
    class DomainEventBase : public DomainEvent {
    public:
        [[nodiscard]] std::type_index GetTypeIndex() const noexcept override {
            return std::type_index(typeid(TDerived));
        }

        [[nodiscard]] std::wstring GetEventType() const noexcept override {
            return TDerived::StaticEventType();
        }

        [[nodiscard]] std::unique_ptr<DomainEvent> Clone() const override {
            return std::make_unique<TDerived>(static_cast<const TDerived&>(*this));
        }
    };

}
