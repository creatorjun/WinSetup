#pragma once

#include <functional>
#include <optional>

namespace winsetup::abstractions {

    template<typename T>
    class IMessageQueue {
    public:
        virtual ~IMessageQueue() = default;

        virtual void Enqueue(T message) = 0;
        virtual std::optional<T> Dequeue() = 0;
        virtual std::optional<T> TryDequeue() = 0;
        virtual bool IsEmpty() const = 0;
        virtual size_t Size() const = 0;
        virtual void Clear() = 0;
    };

}
