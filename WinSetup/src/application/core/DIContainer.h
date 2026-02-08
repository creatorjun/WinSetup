// src/application/core/DIContainer.h
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <any>
#include <shared_mutex>
#include "../../domain/primitives/Expected.h"
#include "../../domain/primitives/Error.h"

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
            std::unique_lock lock(m_mutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>();
                };

            m_registrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::unique_lock lock(m_mutex);

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

            m_registrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::unique_lock lock(m_mutex);

            auto typeIndex = std::type_index(typeid(TInterface));
            m_singletons[typeIndex] = instance;

            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
                };

            m_registrations[typeIndex] = { factory, ServiceLifetime::Singleton };
        }

        template<typename TInterface>
        [[nodiscard]] domain::Expected<std::shared_ptr<TInterface>> Resolve() {
            auto typeIndex = std::type_index(typeid(TInterface));

            {
                std::shared_lock readLock(m_mutex);

                auto regIt = m_registrations.find(typeIndex);
                if (regIt == m_registrations.end()) {
                    return domain::Error{
                        L"Service not registered: " + GetTypeName<TInterface>(),
                        0,
                        domain::ErrorCategory::System
                    };
                }

                if (regIt->second.lifetime == ServiceLifetime::Singleton) {
                    auto singletonIt = m_singletons.find(typeIndex);
                    if (singletonIt != m_singletons.end()) {
                        return std::static_pointer_cast<TInterface>(singletonIt->second);
                    }
                }
            }

            std::unique_lock writeLock(m_mutex);

            auto regIt = m_registrations.find(typeIndex);
            if (regIt == m_registrations.end()) {
                return domain::Error{
                    L"Service not registered: " + GetTypeName<TInterface>(),
                    0,
                    domain::ErrorCategory::System
                };
            }

            const auto& [factory, lifetime] = regIt->second;

            if (lifetime == ServiceLifetime::Singleton) {
                auto singletonIt = m_singletons.find(typeIndex);
                if (singletonIt != m_singletons.end()) {
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

                m_singletons[typeIndex] = instance;
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

        template<typename TInterface>
        [[nodiscard]] bool IsRegistered() const {
            std::shared_lock lock(m_mutex);
            return m_registrations.find(std::type_index(typeid(TInterface))) != m_registrations.end();
        }

        void Clear() {
            std::unique_lock lock(m_mutex);
            m_singletons.clear();
            m_registrations.clear();
        }

    private:
        template<typename T>
        static std::wstring GetTypeName() {
            const char* name = typeid(T).name();
            std::wstring wname;
            wname.reserve(strlen(name));
            for (size_t i = 0; name[i] != '\0'; ++i) {
                wname.push_back(static_cast<wchar_t>(name[i]));
            }
            return wname;
        }

        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime;
        };

        std::unordered_map<std::type_index, Registration> m_registrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> m_singletons;
        mutable std::shared_mutex m_mutex;
    };

}
