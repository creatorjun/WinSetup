// src/abstractions/reactive/IProperty.h 
#pragma once

#include <memory>
#include <functional>
#include "IObservable.h"

namespace winsetup::abstractions {

    enum class PropertyMode {
        ReadOnly,
        WriteOnly,
        ReadWrite
    };

    template<typename T>
    class IProperty : public IObservable<T> {
    public:
        virtual ~IProperty() = default;

        [[nodiscard]] virtual const T& Get() const = 0;

        virtual void Set(const T& value) = 0;

        virtual void Set(T&& value) = 0;

        [[nodiscard]] virtual PropertyMode GetMode() const noexcept = 0;

        [[nodiscard]] virtual bool CanRead() const noexcept {
            const auto mode = GetMode();
            return mode == PropertyMode::ReadOnly || mode == PropertyMode::ReadWrite;
        }

        [[nodiscard]] virtual bool CanWrite() const noexcept {
            const auto mode = GetMode();
            return mode == PropertyMode::WriteOnly || mode == PropertyMode::ReadWrite;
        }

        [[nodiscard]] virtual bool HasValue() const noexcept = 0;

        virtual void Reset() = 0;

        [[nodiscard]] virtual std::shared_ptr<ISubscription> SubscribeToChanges(
            std::function<void(const T& oldValue, const T& newValue)> onChange
        ) = 0;

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<IProperty<TResult>> Transform(
            std::function<TResult(const T&)> getter,
            std::function<T(const TResult&)> setter = nullptr
        ) {
            return TransformImpl<TResult>(std::move(getter), std::move(setter));
        }

        [[nodiscard]] std::shared_ptr<IProperty<T>> WithValidation(
            std::function<bool(const T&)> validator
        ) {
            return WithValidationImpl(std::move(validator));
        }

        [[nodiscard]] std::shared_ptr<IProperty<T>> AsReadOnly() {
            return AsReadOnlyImpl();
        }

        void BindTo(std::shared_ptr<IProperty<T>> target) {
            BindToImpl(std::move(target));
        }

        void BindTwoWay(std::shared_ptr<IProperty<T>> target) {
            BindTwoWayImpl(std::move(target));
        }

        IProperty& operator=(const T& value) {
            Set(value);
            return *this;
        }

        IProperty& operator=(T&& value) {
            Set(std::move(value));
            return *this;
        }

        operator const T& () const {
            return Get();
        }

    protected:
        template<typename TResult>
        [[nodiscard]] virtual std::shared_ptr<IProperty<TResult>> TransformImpl(
            std::function<TResult(const T&)> getter,
            std::function<T(const TResult&)> setter
        ) = 0;

        [[nodiscard]] virtual std::shared_ptr<IProperty<T>> WithValidationImpl(
            std::function<bool(const T&)> validator
        ) = 0;

        [[nodiscard]] virtual std::shared_ptr<IProperty<T>> AsReadOnlyImpl() = 0;

        virtual void BindToImpl(std::shared_ptr<IProperty<T>> target) = 0;
        virtual void BindTwoWayImpl(std::shared_ptr<IProperty<T>> target) = 0;
    };

}
