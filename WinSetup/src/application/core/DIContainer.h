#pragma once
#include <domain/primitives/Expected.h>
#include <domain/primitives/Error.h>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

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
            mRegistrations[std::type_index(typeid(TInterface))] = {
                []() -> std::shared_ptr<void> {
                    return std::make_shared<TImplementation>();
                },
                lifetime
            };
        }

        template<typename TInterface, typename TImplementation, typename... TDeps>
        void RegisterWithDependencies(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
            std::unique_lock lock(mMutex);
            mRegistrations[std::type_index(typeid(TInterface))] = {
                [this]() -> std::shared_ptr<void> {
                    auto resolveDep = [this]<typename T>() -> std::shared_ptr<T> {
                        auto result = Resolve<T>();
                        if (!result.HasValue())
                            throw std::runtime_error(
                                "Critical: Failed to resolve dependency: "
                                + std::string(typeid(T).name()));
                        return result.Value();
                    };
                    auto deps = std::make_tuple(resolveDep.template operator() < TDeps > ()...);
                    return std::apply(
                        [](auto&&... args) {
                            return std::static_pointer_cast<void>(
                                std::make_shared<TImplementation>(
                                    std::forward<decltype(args)>(args)...));
                        }, deps);
                },
                lifetime
            };
        }

        template<typename TInterface>
        void RegisterInstance(std::shared_ptr<TInterface> instance) {
            std::unique_lock lock(mMutex);
            auto typeIndex = std::type_index(typeid(TInterface));
            mRegistrations[typeIndex] = {
                [instance]() -> std::shared_ptr<void> { return instance; },
                ServiceLifetime::Singleton
            };
            mSingletons[typeIndex] = std::static_pointer_cast<void>(instance);
        }

        template<typename TInterface>
        [[nodiscard]] domain::Expected<std::shared_ptr<TInterface>> Resolve() {
            const auto typeIndex = std::type_index(typeid(TInterface));

            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime lifetime{};
            {
                std::shared_lock readLock(mMutex);

                auto sit = mSingletons.find(typeIndex);
                if (sit != mSingletons.end())
                    return std::static_pointer_cast<TInterface>(sit->second);

                auto rit = mRegistrations.find(typeIndex);
                if (rit == mRegistrations.end())
                    return domain::Error{
                        L"Service not registered: " + GetTypeName<TInterface>(),
                        0, domain::ErrorCategory::System };

                factory = rit->second.factory;
                lifetime = rit->second.lifetime;
            }

            if (lifetime != ServiceLifetime::Singleton) {
                auto instance = factory();
                if (!instance)
                    return domain::Error{
                        L"Factory returned null: " + GetTypeName<TInterface>(),
                        0, domain::ErrorCategory::System };
                return std::static_pointer_cast<TInterface>(instance);
            }

            std::unique_lock writeLock(mMutex);

            auto sit = mSingletons.find(typeIndex);
            if (sit != mSingletons.end())
                return std::static_pointer_cast<TInterface>(sit->second);

            auto instance = factory();
            if (!instance)
                return domain::Error{
                    L"Factory returned null: " + GetTypeName<TInterface>(),
                    0, domain::ErrorCategory::System };

            mSingletons[typeIndex] = instance;
            return std::static_pointer_cast<TInterface>(instance);
        }

        void Clear() {
            std::unique_lock lock(mMutex);
            mSingletons.clear();
            mRegistrations.clear();
        }

    private:
        struct Registration {
            std::function<std::shared_ptr<void>()> factory;
            ServiceLifetime                         lifetime = ServiceLifetime::Singleton;
        };

        template<typename T>
        [[nodiscard]] static std::wstring GetTypeName() {
            const char* src = typeid(T).name();
            return std::wstring(src, src + std::char_traits<char>::length(src));
        }

        std::unordered_map<std::type_index, Registration>          mRegistrations;
        std::unordered_map<std::type_index, std::shared_ptr<void>> mSingletons;
        mutable std::shared_mutex                                   mMutex;
    };

} // namespace winsetup::application
