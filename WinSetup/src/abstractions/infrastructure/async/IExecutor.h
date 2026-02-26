// src/abstractions/infrastructure/async/IExecutor.h
#pragma once
#include <functional>

namespace winsetup::abstractions {

    class IExecutor {
    public:
        virtual ~IExecutor() = default;
        virtual void Post(std::function<void()> task) = 0;
    };

} // namespace winsetup::abstractions
