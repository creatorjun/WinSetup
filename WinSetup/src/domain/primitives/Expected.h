// src/domain/primitives/Expected.h

#pragma once

#include "Error.h"
#include <variant>
#include <type_traits>
#include <stdexcept>

namespace winsetup::domain {

    template<typename T, typename E = Error>
    class Expected {
    public:
        Expected(const Expected&) = default;
        Expected(Expected&&) noexcept = default;
        Expected& operator=(const Expected&) = default;
        Expected& operator=(Expected&&) noexcept = default;

        static Expected Success(T value) {
            return Expected(std::move(value));
        }

        static Expected Failure(E error) {
            Expected result;
            result.data_ = std::move(error);
            return result;
        }

        bool HasValue() const noexcept {
            return std::holds_alternative<T>(data_);
        }

        bool HasError() const noexcept {
            return std::holds_alternative<E>(data_);
        }

        const T& Value() const& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::get<T>(data_);
        }

        T& Value()& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::get<T>(data_);
        }

        T&& Value()&& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::get<T>(std::move(data_));
        }

        const E& GetError() const& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::get<E>(data_);
        }

        E& GetError()& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::get<E>(data_);
        }

        E&& GetError()&& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::get<E>(std::move(data_));
        }

        T ValueOr(T defaultValue) const& {
            return HasValue() ? Value() : std::move(defaultValue);
        }

        T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(*this).Value() : std::move(defaultValue);
        }

        template<typename F>
        auto Map(F&& func) const& -> Expected<decltype(func(std::declval<T>())), E> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Expected<U, E>::Success(func(Value()));
            }
            return Expected<U, E>::Failure(GetError());
        }

        template<typename F>
        auto Map(F&& func) && -> Expected<decltype(func(std::declval<T>())), E> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Expected<U, E>::Success(func(std::move(*this).Value()));
            }
            return Expected<U, E>::Failure(std::move(*this).GetError());
        }

        template<typename F>
        auto FlatMap(F&& func) const& -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(Value());
            }
            using ReturnType = decltype(func(std::declval<T>()));
            return ReturnType::Failure(GetError());
        }

        template<typename F>
        auto FlatMap(F&& func) && -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(std::move(*this).Value());
            }
            using ReturnType = decltype(func(std::declval<T>()));
            return ReturnType::Failure(std::move(*this).GetError());
        }

        template<typename F>
        Expected MapError(F&& func) const& {
            if (HasError()) {
                return Expected::Failure(func(GetError()));
            }
            return Expected::Success(Value());
        }

        explicit operator bool() const noexcept {
            return HasValue();
        }

    private:
        Expected() = default;
        explicit Expected(T value) : data_(std::move(value)) {}

        std::variant<T, E> data_;
    };

}
