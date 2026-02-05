// src/abstractions/reactive/IObservable.h 
#pragma once

#include <functional>
#include <memory>
#include <exception>

namespace winsetup::abstractions {

    using SubscriptionId = size_t;

    template<typename T>
    class IObserver {
    public:
        virtual ~IObserver() = default;

        virtual void OnNext(const T& value) = 0;
        virtual void OnError(std::exception_ptr error) = 0;
        virtual void OnCompleted() = 0;
    };

    class ISubscription {
    public:
        virtual ~ISubscription() = default;

        virtual void Unsubscribe() = 0;
        [[nodiscard]] virtual bool IsUnsubscribed() const noexcept = 0;
    };

    template<typename T>
    class IObservable {
    public:
        virtual ~IObservable() = default;

        [[nodiscard]] virtual std::shared_ptr<ISubscription> Subscribe(
            std::shared_ptr<IObserver<T>> observer
        ) = 0;

        [[nodiscard]] std::shared_ptr<ISubscription> Subscribe(
            std::function<void(const T&)> onNext,
            std::function<void(std::exception_ptr)> onError = nullptr,
            std::function<void()> onCompleted = nullptr
        ) {
            return SubscribeCallbacks(
                std::move(onNext),
                std::move(onError),
                std::move(onCompleted)
            );
        }

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<IObservable<TResult>> Map(
            std::function<TResult(const T&)> transform
        ) {
            return MapImpl<TResult>(std::move(transform));
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> Filter(
            std::function<bool(const T&)> predicate
        ) {
            return FilterImpl(std::move(predicate));
        }

        template<typename TResult>
        [[nodiscard]] std::shared_ptr<IObservable<TResult>> FlatMap(
            std::function<std::shared_ptr<IObservable<TResult>>(const T&)> transform
        ) {
            return FlatMapImpl<TResult>(std::move(transform));
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> Take(size_t count) {
            return TakeImpl(count);
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> Skip(size_t count) {
            return SkipImpl(count);
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> DistinctUntilChanged() {
            return DistinctUntilChangedImpl();
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> Debounce(
            std::chrono::milliseconds timeout
        ) {
            return DebounceImpl(timeout);
        }

        [[nodiscard]] std::shared_ptr<IObservable<T>> Throttle(
            std::chrono::milliseconds interval
        ) {
            return ThrottleImpl(interval);
        }

    protected:
        [[nodiscard]] virtual std::shared_ptr<ISubscription> SubscribeCallbacks(
            std::function<void(const T&)> onNext,
            std::function<void(std::exception_ptr)> onError,
            std::function<void()> onCompleted
        ) = 0;

        template<typename TResult>
        [[nodiscard]] virtual std::shared_ptr<IObservable<TResult>> MapImpl(
            std::function<TResult(const T&)> transform
        ) = 0;

        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> FilterImpl(
            std::function<bool(const T&)> predicate
        ) = 0;

        template<typename TResult>
        [[nodiscard]] virtual std::shared_ptr<IObservable<TResult>> FlatMapImpl(
            std::function<std::shared_ptr<IObservable<TResult>>(const T&)> transform
        ) = 0;

        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> TakeImpl(size_t count) = 0;
        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> SkipImpl(size_t count) = 0;
        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> DistinctUntilChangedImpl() = 0;
        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> DebounceImpl(std::chrono::milliseconds timeout) = 0;
        [[nodiscard]] virtual std::shared_ptr<IObservable<T>> ThrottleImpl(std::chrono::milliseconds interval) = 0;
    };

}
