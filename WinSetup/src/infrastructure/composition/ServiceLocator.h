// src/infrastructure/composition/ServiceLocator.h 
#pragma once

#include <memory>
#include <mutex>
#include "DependencyContainer.h"

namespace winsetup::infrastructure {

    class ServiceLocator {
    public:
        static void Initialize(std::shared_ptr<DependencyContainer> container) {
            std::lock_guard<std::mutex> lock(mutex_);
            instance_ = std::move(container);
        }

        static void Reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            instance_.reset();
        }

        template<typename TInterface>
        [[nodiscard]] static std::shared_ptr<TInterface> Resolve() {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!instance_) {
                return nullptr;
            }

            return instance_->Resolve<TInterface>();
        }

        [[nodiscard]] static bool IsInitialized() {
            std::lock_guard<std::mutex> lock(mutex_);
            return instance_ != nullptr;
        }

        [[nodiscard]] static std::shared_ptr<DependencyContainer> GetContainer() {
            std::lock_guard<std::mutex> lock(mutex_);
            return instance_;
        }

    private:
        ServiceLocator() = delete;
        ~ServiceLocator() = delete;

        static std::mutex mutex_;
        static std::shared_ptr<DependencyContainer> instance_;
    };

    inline std::mutex ServiceLocator::mutex_;
    inline std::shared_ptr<DependencyContainer> ServiceLocator::instance_;

}
