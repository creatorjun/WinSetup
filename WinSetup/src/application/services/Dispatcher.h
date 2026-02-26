#pragma once
#include "abstractions/ui/IUIDispatcher.h"
#include <Windows.h>
#include <functional>
#include <mutex>
#include <queue>

namespace winsetup::application {

    class Dispatcher final : public abstractions::IUIDispatcher {
    public:
        Dispatcher();
        ~Dispatcher() override = default;

        void Post(std::function<void()> action) override;

        void SetTargetHwnd(HWND hwnd);
        void ProcessPending();

        static constexpr UINT WM_DISPATCHER_INVOKE = WM_USER + 100;

    private:
        HWND mTargetHwnd{ nullptr };
        std::mutex mMutex;
        std::queue<std::function<void()>> mQueue;
    };

}
