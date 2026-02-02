#pragma once

#include "Error.h"
#include <variant>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace winsetup::domain {

    template<typename T, typename E = Error>
    class Expected {
    public:
        Expected(const Expected&) = default;
        Expected(Expected&&) noexcept = default;
        Expected& operator=(const Expected&) = default;
        Expected& operator=(Expected&&) noexcept = default;

        static Expected Success(T value) {
            Expected result;
            result.data_ = std::move(value);
            return result;
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
            return std::move(std::get<T>(data_));
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
            return std::move(std::get<E>(data_));
        }

        T ValueOr(T defaultValue) const& {
            return HasValue() ? Value() : std::move(defaultValue);
        }

        T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(*this).Value() : std::move(defaultValue);
        }

        template<typename F>
        auto Map(F&& func) const& -> Expected<std::invoke_result_t<F, const T&>, E> {
            using U = std::invoke_result_t<F, const T&>;
            if (HasValue()) {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), Value()));
            }
            return Expected<U, E>::Failure(GetError());
        }

        template<typename F>
        auto Map(F&& func) && -> Expected<std::invoke_result_t<F, T&&>, E> {
            using U = std::invoke_result_t<F, T&&>;
            if (HasValue()) {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), std::move(*this).Value()));
            }
            return Expected<U, E>::Failure(std::move(*this).GetError());
        }

        template<typename F>
        auto FlatMap(F&& func) const& -> std::invoke_result_t<F, const T&> {
            if (HasValue()) {
                return std::invoke(std::forward<F>(func), Value());
            }
            using ReturnType = std::invoke_result_t<F, const T&>;
            return ReturnType::Failure(GetError());
        }

        template<typename F>
        auto FlatMap(F&& func) && -> std::invoke_result_t<F, T&&> {
            if (HasValue()) {
                return std::invoke(std::forward<F>(func), std::move(*this).Value());
            }
            using ReturnType = std::invoke_result_t<F, T&&>;
            return ReturnType::Failure(std::move(*this).GetError());
        }

        template<typename F>
        auto MapError(F&& func) const& -> Expected<T, std::invoke_result_t<F, const E&>> {
            using NewE = std::invoke_result_t<F, const E&>;
            if (HasError()) {
                return Expected<T, NewE>::Failure(std::invoke(std::forward<F>(func), GetError()));
            }
            return Expected<T, NewE>::Success(Value());
        }

        template<typename F>
        auto MapError(F&& func) && -> Expected<T, std::invoke_result_t<F, E&&>> {
            using NewE = std::invoke_result_t<F, E&&>;
            if (HasError()) {
                return Expected<T, NewE>::Failure(std::invoke(std::forward<F>(func), std::move(*this).GetError()));
            }
            return Expected<T, NewE>::Success(std::move(*this).Value());
        }

        explicit operator bool() const noexcept {
            return HasValue();
        }

    private:
        Expected() = default;

        std::variant<T, E> data_;
    };

}
