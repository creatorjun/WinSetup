#include "application/services/Dispatcher.h"

namespace winsetup::application {

    Dispatcher::Dispatcher() = default;

    void Dispatcher::SetTargetHwnd(HWND hwnd) {
        mTargetHwnd = hwnd;
    }

    void Dispatcher::Post(std::function<void()> action) {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mQueue.push(std::move(action));
        }
        if (mTargetHwnd)
            PostMessageW(mTargetHwnd, WM_DISPATCHER_INVOKE, 0, 0);
    }

    void Dispatcher::ProcessPending() {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            std::swap(local, mQueue);
        }
        while (!local.empty()) {
            local.front()();
            local.pop();
        }
    }

}
