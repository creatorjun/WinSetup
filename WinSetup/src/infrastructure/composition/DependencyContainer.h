// src/infrastructure/composition/DependencyContainer.h 
#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <string>

namespace winsetup::infrastructure {

    enum class ServiceLifetime {
        Singleton,
        Transient
    };

    class DependencyContainer {
    public:
        DependencyContainer() = default;
        ~DependencyContainer() = default;

        DependencyContainer(const DependencyContainer&) = delete;
        DependencyContainer& operator=(const DependencyContainer&) = delete;
        DependencyContainer(DependencyContainer&&) = delete;
        DependencyContainer& operator=(DependencyContainer&&) = delete;

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void Register(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto typeId = std::type_index(typeid(TInterface));

            if (lifetime == ServiceLifetime::Singleton) {
                factories_[typeId] = [this]() -> std::shared_ptr<void> {
                    return std::static_pointer_cast<void>(
                        std::make_shared<TImplementation>(
                            Resolve<TDeps>()...
                        )
                    );
                    };
                lifetimes_[typeId] = ServiceLifetime::Singleton;
            }
            else {
                factories_[typeId] = [this]() -> std::shared_ptr<void> {
                    return std::static_pointer_cast<void>(
                        std::make_shared<TImplementation>(
                            Resolve<TDeps>()...
                        )
                    );
                    };
                lifetimes_[typeId] = ServiceLifetime::Transient;
            }
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto typeId = std::type_index(typeid(TInterface));
            singletons_[typeId] = std::static_pointer_cast<void>(instance);
            lifetimes_[typeId] = ServiceLifetime::Singleton;
        }

        template<typename TInterface>
        void RegisterFactory(
            std::function<std::shared_ptr<TInterface>()> factory,
            ServiceLifetime lifetime = ServiceLifetime::Singleton
        ) {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto typeId = std::type_index(typeid(TInterface));

            factories_[typeId] = [factory]() -> std::shared_ptr<void> {
                return std::static_pointer_cast<void>(factory());
                };
            lifetimes_[typeId] = lifetime;
        }

        template<typename TInterface>
        [[nodiscard]] std::shared_ptr<TInterface> Resolve() {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto typeId = std::type_index(typeid(TInterface));

            if (lifetimes_[typeId] == ServiceLifetime::Singleton) {
                auto it = singletons_.find(typeId);
                if (it != singletons_.end()) {
                    return std::static_pointer_cast<TInterface>(it->second);
                }

                auto factoryIt = factories_.find(typeId);
                if (factoryIt != factories_.end()) {
                    auto instance = factoryIt->second();
                    singletons_[typeId] = instance;
                    return std::static_pointer_cast<TInterface>(instance);
                }
            }
            else {
                auto factoryIt = factories_.find(typeId);
                if (factoryIt != factories_.end()) {
                    auto instance = factoryIt->second();
                    return std::static_pointer_cast<TInterface>(instance);
                }
            }

            return nullptr;
        }

        template<typename TInterface>
        [[nodiscard]] bool IsRegistered() const {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto typeId = std::type_index(typeid(TInterface));
            return factories_.find(typeId) != factories_.end() ||
                singletons_.find(typeId) != singletons_.end();
        }

        void Clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            singletons_.clear();
            factories_.clear();
            lifetimes_.clear();
        }

        [[nodiscard]] size_t GetRegisteredCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return factories_.size();
        }

        [[nodiscard]] size_t GetSingletonCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return singletons_.size();
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
        std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;
        std::unordered_map<std::type_index, ServiceLifetime> lifetimes_;
    };

}
