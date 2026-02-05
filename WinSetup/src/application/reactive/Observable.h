// src/application/reactive/Observable.h 
#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <algorithm>
#include <atomic>
#include "../../abstractions/reactive/IObservable.h"

namespace winsetup::application {

    template<typename T>
    class Subscription : public abstractions::ISubscription {
    public:
        explicit Subscription(std::function<void()> unsubscribeFunc)
            : unsubscribeFunc_(std::move(unsubscribeFunc))
            , unsubscribed_(false) {
        }

        ~Subscription() override {
            Unsubscribe();
        }

        void Unsubscribe() override {
            bool expected = false;
            if (unsubscribed_.compare_exchange_strong(expected, true)) {
                if (unsubscribeFunc_) {
                    unsubscribeFunc_();
                }
            }
        }

        [[nodiscard]] bool IsUnsubscribed() const noexcept override {
            return unsubscribed_.load(std::memory_order_acquire);
        }

    private:
        std::function<void()> unsubscribeFunc_;
        std::atomic<bool> unsubscribed_;
    };

    template<typename T>
    class CallbackObserver : public abstractions::IObserver<T> {
    public:
        CallbackObserver(
            std::function<void(const T&)> onNext,
            std::function<void(std::exception_ptr)> onError,
            std::function<void()> onCompleted
        )
            : onNext_(std::move(onNext))
            , onError_(std::move(onError))
            , onCompleted_(std::move(onCompleted)) {
        }

        void OnNext(const T& value) override {
            if (onNext_) {
                onNext_(value);
            }
        }

        void OnError(std::exception_ptr error) override {
            if (onError_) {
                onError_(error);
            }
        }

        void OnCompleted() override {
            if (onCompleted_) {
                onCompleted_();
            }
        }

    private:
        std::function<void(const T&)> onNext_;
        std::function<void(std::exception_ptr)> onError_;
        std::function<void()> onCompleted_;
    };

    template<typename T>
    class Observable : public abstractions::IObservable<T> {
    public:
        Observable() = default;
        virtual ~Observable() = default;

        [[nodiscard]] std::shared_ptr<abstractions::ISubscription> Subscribe(
            std::shared_ptr<abstractions::IObserver<T>> observer
        ) override {
            std::lock_guard<std::mutex> lock(mutex_);

            const auto id = nextId_++;
            observers_[id] = observer;

            return std::make_shared<Subscription<T>>([this, id]() {
                std::lock_guard<std::mutex> lock(mutex_);
                observers_.erase(id);
                });
        }

        void EmitNext(const T& value) {
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

        void EmitError(std::exception_ptr error) {
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
                    observer->OnError(error);
                }
                catch (...) {
                }
            }
        }

        void EmitCompleted() {
            std::vector<std::shared_ptr<abstractions::IObserver<T>>> currentObservers;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                currentObservers.reserve(observers_.size());
                for (const auto& [id, observer] : observers_) {
                    currentObservers.push_back(observer);
                }
                observers_.clear();
            }

            for (const auto& observer : currentObservers) {
                try {
                    observer->OnCompleted();
                }
                catch (...) {
                }
            }
        }

        [[nodiscard]] size_t GetObserverCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return observers_.size();
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
        mutable std::mutex mutex_;
        std::unordered_map<size_t, std::shared_ptr<abstractions::IObserver<T>>> observers_;
        size_t nextId_{ 0 };
    };

}
