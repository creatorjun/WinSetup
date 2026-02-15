// src/application/core/DIContainer.h
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <any>
#include <shared_mutex>
#include <string>
#include <codecvt>
#include <locale>
#include <domain/primitives/Expected.h>
#include <domain/primitives/Error.h>

namespace winsetup::application {

    enum class ServiceLifetime {
        Singleton,
        Transient,
        Scoped
    };

    class DIContainer {
    public:
        template<typename TInterface, typename TImplementation>
        void Register(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::unique_lock lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>();
                };

            mregistrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::unique_lock lock(mMutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                auto resolveDep = [this]<typename T>() -> std::shared_ptr<T> {
                    auto result = Resolve<T>();
                    if (!result.HasValue()) {
                        return nullptr;
                    }
                    return result.Value();
                };

                auto deps = std::make_tuple(resolveDep.template operator() < TDeps > ()...);

                bool allResolved = std::apply([](auto&&... args) {
                    return (... && (args != nullptr));
                    }, deps);

                if (!allResolved) {
                    return nullptr;
                }

                return std::apply([](auto&&... args) {
                    return std::make_shared<TImplementation>(std::forward<decltype(args)>(args)...);
                    }, deps);
                };

            mregistrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::unique_lock lock(mMutex);

            auto typeIndex = std::type_index(typeid(TInterface));
            msingletons[typeIndex] = instance;

            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
                };

            mregistrations[typeIndex] = { factory, ServiceLifetime::Singleton };
        }

        template<typename TInterface>
        [[nodiscard]] domain::Expected<std::shared_ptr<TInterface>> Resolve() {
            auto typeIndex = std::type_index(typeid(TInterface));

            {
                std::shared_lock readLock(mMutex);

                auto regIt = mregistrations.find(typeIndex);
                if (regIt == mregistrations.end()) {
                    return domain::Error{
                        L"Service not registered: " + GetTypeName<TInterface>(),
                        0,
                        domain::ErrorCategory::System
                    };
                }

                const auto& [factory, lifetime] = regIt->second;

                if (lifetime == ServiceLifetime::Singleton) {
                    auto singletonIt = msingletons.find(typeIndex);
                    if (singletonIt != msingletons.end()) {
                        return std::static_pointer_cast<TInterface>(singletonIt->second);
                    }
                }
            }

            std::unique_lock writeLock(mMutex);

            auto regIt = mregistrations.find(typeIndex);
            if (regIt == mregistrations.end()) {
                return domain::Error{
                    L"Service not registered: " + GetTypeName<TInterface>(),
                    0,
                    domain::ErrorCategory::System
                };
            }

            const auto& [factory, lifetime] = regIt->second;

            if (lifetime == ServiceLifetime::Singleton) {
                auto singletonIt = msingletons.find(typeIndex);
                if (singletonIt != msingletons.end()) {
                    return std::static_pointer_cast<TInterface>(singletonIt->second);
                }

                auto instance = factory();
                if (!instance) {
                    return domain::Error{
                        L"Failed to create instance: " + GetTypeName<TInterface>(),
                        0,
                        domain::ErrorCategory::System
                    };
                }

                msingletons[typeIndex] = instance;
                return std::static_pointer_cast<TInterface>(instance);
            }

            auto instance = factory();
            if (!instance) {
                return domain::Error{
                    L"Failed to create instance: " + GetTypeName<TInterface>(),
                    0,
                    domain::ErrorCategory::System
                };
            }

            return std::static_pointer_cast<TInterface>(instance);
        }

        void Clear() {
            std::unique_lock lock(mMutex);
            msingletons.clear();
            mregistrations.clear();
        }

    private:
        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime = ServiceLifetime::Singleton;
        };

        template<typename T>
        [[nodiscard]] static std::wstring GetTypeName() {
            const char* name = typeid(T).name();
            size_t len = 0;
            while (name[len] != '\0') {
                len++;
            }

            std::wstring result;
            result.reserve(len);
            for (size_t i = 0; i < len; ++i) {
                result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(name[i])));
            }
            return result;
        }

        std::unordered_map<std::type_index, Registration> mregistrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> msingletons;
        mutable std::shared_mutex mMutex;
    };

}
