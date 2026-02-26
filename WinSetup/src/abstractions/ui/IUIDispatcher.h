#pragma once
#include <functional>

namespace winsetup::abstractions {

    class IUIDispatcher {
    public:
        virtual ~IUIDispatcher() = default;
        virtual void Post(std::function<void()> action) = 0;
    };

}
