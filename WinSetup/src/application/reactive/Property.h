// src/application/reactive/Property.h 
#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <optional>
#include <functional>
#include <abstractions/reactive/IProperty.h>
#include "Observable.h"

namespace winsetup::application {

    template<typename T>
    class Property : public abstractions::IProperty<T>,
        public std::enable_shared_from_this<Property<T>> {
    public:
        Property()
            : mode_(abstractions::PropertyMode::ReadWrite)
            , hasValue_(false) {
        }

        explicit Property(const T& initialValue)
            : value_(initialValue)
            , mode_(abstractions::PropertyMode::ReadWrite)
            , hasValue_(true) {
        }

        explicit Property(T&& initialValue)
            : value_(std::move(initialValue))
            , mode_(abstractions::PropertyMode::ReadWrite)
            , hasValue_(true) {
        }

        Property(const T& initialValue, abstractions::PropertyMode mode)
            : value_(initialValue)
            , mode_(mode)
            , hasValue_(true) {
        }

        ~Property() override = default;

        [[nodiscard]] const T& Get() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!hasValue_) {
                throw std::runtime_error("Property has no value");
            }
            return value_;
        }

        void Set(const T& newValue) override {
            if (!this->CanWrite()) {
                throw std::runtime_error("Property is read-only");
            }

            if (validator_ && !validator_(newValue)) {
                throw std::invalid_argument("Property validation failed");
            }

            T oldValue;
            bool changed = false;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!hasValue_ || value_ != newValue) {
                    oldValue = hasValue_ ? value_ : T{};
                    value_ = newValue;
                    hasValue_ = true;
                    changed = true;
                }
            }

            if (changed) {
                NotifyObservers(newValue);
                NotifyChangeObservers(oldValue, newValue);
            }
        }

        void Set(T&& newValue) override {
            if (!this->CanWrite()) {
                throw std::runtime_error("Property is read-only");
            }

            if (validator_ && !validator_(newValue)) {
                throw std::invalid_argument("Property validation failed");
            }

            T oldValue;
            bool changed = false;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!hasValue_ || value_ != newValue) {
                    oldValue = hasValue_ ? value_ : T{};
                    value_ = std::move(newValue);
                    hasValue_ = true;
                    changed = true;
                }
            }

            if (changed) {
                NotifyObservers(value_);
                NotifyChangeObservers(oldValue, value_);
            }
        }

        [[nodiscard]] abstractions::PropertyMode GetMode() const noexcept override {
            return mode_;
        }

        [[nodiscard]] bool HasValue() const noexcept override {
            std::lock_guard<std::mutex> lock(mutex_);
            return hasValue_;
        }

        void Reset() override {
            T oldValue;
            bool hadValue = false;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (hasValue_) {
                    oldValue = std::move(value_);
                    hadValue = true;
                }
                value_ = T{};
                hasValue_ = false;
            }

            if (hadValue) {
                NotifyObservers(T{});
                NotifyChangeObservers(oldValue, T{});
            }
        }

        [[nodiscard]] std::shared_ptr<abstractions::ISubscription> Subscribe(
            std::shared_ptr<abstractions::IObserver<T>> observer
        ) override {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto id = nextObserverId_++;
            observers_[id] = observer;

            if (hasValue_) {
                try {
                    observer->OnNext(value_);
                }
                catch (...) {
                }
            }

            return std::make_shared<Subscription<T>>([this, id]() {
                std::lock_guard<std::mutex> lock(mutex_);
                observers_.erase(id);
                });
        }

        [[nodiscard]] std::shared_ptr<abstractions::ISubscription> SubscribeToChanges(
            std::function<void(const T& oldValue, const T& newValue)> onChange
        ) override {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto id = nextChangeObserverId_++;
            changeObservers_[id] = std::move(onChange);

            return std::make_shared<Subscription<T>>([this, id]() {
                std::lock_guard<std::mutex> lock(mutex_);
                changeObservers_.erase(id);
                });
        }

    protected:
        [[nodiscard]] std::shared_ptr<abstractions::ISubscription> SubscribeCallbacks(
            std::function<void(const T&)> onNext,
            std::function<void(std::exception_ptr)> onError,
            std::function<void()> onCompleted
        ) override {
            auto observer = std::make_shared<CallbackObserver<T>>(
                std::move(onNext),
                std::move(onError),
                std::move(onCompleted)
            );
            return Subscribe(observer);
        }

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<abstractions::IProperty<TResult>> TransformImpl(
            std::function<TResult(const T&)> getter,
            std::function<T(const TResult&)> setter
        ) override;

        [[nodiscard]] std::shared_ptr<abstractions::IProperty<T>> WithValidationImpl(
            std::function<bool(const T&)> validator
        ) override {
            auto newProperty = std::make_shared<Property<T>>();
            if (hasValue_) {
                newProperty->Set(value_);
            }
            newProperty->validator_ = std::move(validator);
            return newProperty;
        }

        [[nodiscard]] std::shared_ptr<abstractions::IProperty<T>> AsReadOnlyImpl() override {
            auto readOnly = std::make_shared<Property<T>>();
            readOnly->mode_ = abstractions::PropertyMode::ReadOnly;
            if (hasValue_) {
                readOnly->value_ = value_;
                readOnly->hasValue_ = true;
            }

            SubscribeToChanges([readOnly](const T&, const T& newValue) {
                std::lock_guard<std::mutex> lock(readOnly->mutex_);
                readOnly->value_ = newValue;
                readOnly->hasValue_ = true;
                });

            return readOnly;
        }

        void BindToImpl(std::shared_ptr<abstractions::IProperty<T>> target) override {
            if (!target) {
                return;
            }

            SubscribeToChanges([target](const T&, const T& newValue) {
                if (target->CanWrite()) {
                    target->Set(newValue);
                }
                });
        }

        void BindTwoWayImpl(std::shared_ptr<abstractions::IProperty<T>> target) override {
            if (!target) {
                return;
            }

            BindToImpl(target);

            target->SubscribeToChanges([weak = this->weak_from_this()](const T&, const T& newValue) {
                if (auto strong = weak.lock()) {
                    if (strong->CanWrite()) {
                        strong->Set(newValue);
                    }
                }
                });
        }

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<TResult>> MapImpl(
            std::function<TResult(const T&)> transform
        ) override;

        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> FilterImpl(
            std::function<bool(const T&)> predicate
        ) override;

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<TResult>> FlatMapImpl(
            std::function<std::shared_ptr<abstractions::IObservable<TResult>>(const T&)> transform
        ) override;

        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> TakeImpl(size_t count) override;
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> SkipImpl(size_t count) override;
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> DistinctUntilChangedImpl() override;
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> DebounceImpl(std::chrono::milliseconds timeout) override;
        [[nodiscard]] std::shared_ptr<abstractions::IObservable<T>> ThrottleImpl(std::chrono::milliseconds interval) override;

    private:
        void NotifyObservers(const T& value) {
            std::vector<std::shared_ptr<abstractions::IObserver<T>>> currentObservers;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                currentObservers.reserve(observers_.size());
                for (const auto& [id, observer] : observers_) {
                    currentObservers.push_back(observer);
                }
            }

            for (const auto& observer : currentObservers) {
                try {
                    observer->OnNext(value);
                }
                catch (...) {
                }
            }
        }

        void NotifyChangeObservers(const T& oldValue, const T& newValue) {
            std::vector<std::function<void(const T&, const T&)>> currentChangeObservers;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                currentChangeObservers.reserve(changeObservers_.size());
                for (const auto& [id, observer] : changeObservers_) {
                    currentChangeObservers.push_back(observer);
                }
            }

            for (const auto& observer : currentChangeObservers) {
                try {
                    observer(oldValue, newValue);
                }
                catch (...) {
                }
            }
        }

        mutable std::mutex mutex_;
        T value_;
        abstractions::PropertyMode mode_;
        bool hasValue_;
        std::function<bool(const T&)> validator_;

        std::unordered_map<size_t, std::shared_ptr<abstractions::IObserver<T>>> observers_;
        size_t nextObserverId_{ 0 };

        std::unordered_map<size_t, std::function<void(const T&, const T&)>> changeObservers_;
        size_t nextChangeObserverId_{ 0 };
    };

    template<typename T>
    [[nodiscard]] inline std::shared_ptr<Property<T>> MakeProperty() {
        return std::make_shared<Property<T>>();
    }

    template<typename T>
    [[nodiscard]] inline std::shared_ptr<Property<T>> MakeProperty(T value) {
        return std::make_shared<Property<T>>(std::move(value));
    }

    template<typename T>
    [[nodiscard]] inline std::shared_ptr<Property<T>> MakeReadOnlyProperty(T value) {
        return std::make_shared<Property<T>>(
            std::move(value),
            abstractions::PropertyMode::ReadOnly
        );
    }

}
