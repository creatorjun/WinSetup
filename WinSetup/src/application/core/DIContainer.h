// src/application/core/DIContainer.h

#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <any>
#include <mutex>
#include <stdexcept>

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
            std::lock_guard lock(m_mutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>();
                };

            m_registrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::lock_guard lock(m_mutex);

            auto factory = [this]() -> std::shared_ptr<void> {
                return std::make_shared<TImplementation>(
                    Resolve<TDeps>()...
                );
                };

            m_registrations[std::type_index(typeid(TInterface))] = { factory, lifetime };
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::lock_guard lock(m_mutex);

            auto typeIndex = std::type_index(typeid(TInterface));

            m_singletons[typeIndex] = instance;

            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
                };

            m_registrations[typeIndex] = { factory, ServiceLifetime::Singleton };
        }

        template<typename TInterface>
        std::shared_ptr<TInterface> Resolve() {
            std::lock_guard lock(m_mutex);

            auto typeIndex = std::type_index(typeid(TInterface));

            auto it = m_registrations.find(typeIndex);
            if (it == m_registrations.end()) {
                throw std::runtime_error("Service not registered");
            }

            const auto& [factory, lifetime] = it->second;

            if (lifetime == ServiceLifetime::Singleton) {
                auto singletonIt = m_singletons.find(typeIndex);
                if (singletonIt != m_singletons.end()) {
                    return std::static_pointer_cast<TInterface>(singletonIt->second);
                }

                auto instance = factory();
                m_singletons[typeIndex] = instance;
                return std::static_pointer_cast<TInterface>(instance);
            }

            return std::static_pointer_cast<TInterface>(factory());
        }

        template<typename TInterface>
        bool IsRegistered() const {
            std::lock_guard lock(m_mutex);
            return m_registrations.find(std::type_index(typeid(TInterface))) != m_registrations.end();
        }

        void Clear() {
            std::lock_guard lock(m_mutex);
            m_singletons.clear();
            m_registrations.clear();
        }

    private:
        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime;
        };

        std::unordered_map<std::type_index, Registration> m_registrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> m_singletons;
        mutable std::mutex m_mutex;
    };

}
