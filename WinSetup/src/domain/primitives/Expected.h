#pragma once

#include "Error.h"
#include <variant>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <concepts>

namespace winsetup::domain {

    template<typename T, typename E = Error>
    class Expected {
    public:
        Expected(const Expected&) = default;
        Expected(Expected&&) noexcept = default;
        Expected& operator=(const Expected&) = default;
        Expected& operator=(Expected&&) noexcept = default;

        static constexpr Expected Success(T value) {
            Expected result;
            result.data_ = std::move(value);
            return result;
        }

        static constexpr Expected Failure(E error) {
            Expected result;
            result.data_ = std::move(error);
            return result;
        }

        constexpr bool HasValue() const noexcept {
            return std::holds_alternative<T>(data_);
        }

        constexpr bool HasError() const noexcept {
            return std::holds_alternative<E>(data_);
        }

        constexpr const T& Value() const& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::get<T>(data_);
        }

        constexpr T& Value()& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::get<T>(data_);
        }

        constexpr T&& Value()&& {
            if (!HasValue()) {
                throw std::logic_error("Expected does not contain a value");
            }
            return std::move(std::get<T>(data_));
        }

        constexpr const E& GetError() const& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::get<E>(data_);
        }

        constexpr E& GetError()& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::get<E>(data_);
        }

        constexpr E&& GetError()&& {
            if (!HasError()) {
                throw std::logic_error("Expected does not contain an error");
            }
            return std::move(std::get<E>(data_));
        }

        constexpr T ValueOr(T defaultValue) const& {
            return HasValue() ? Value() : std::move(defaultValue);
        }

        constexpr T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(*this).Value() : std::move(defaultValue);
        }

        template<typename F>
        constexpr auto Map(F&& func) const& -> Expected<std::invoke_result_t<F, const T&>, E> {
            using U = std::invoke_result_t<F, const T&>;
            if (HasValue()) {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), Value()));
            }
            return Expected<U, E>::Failure(GetError());
        }

        template<typename F>
        constexpr auto Map(F&& func) && -> Expected<std::invoke_result_t<F, T&&>, E> {
            using U = std::invoke_result_t<F, T&&>;
            if (HasValue()) {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), std::move(*this).Value()));
            }
            return Expected<U, E>::Failure(std::move(*this).GetError());
        }

        template<typename F>
        constexpr auto FlatMap(F&& func) const& -> std::invoke_result_t<F, const T&> {
            if (HasValue()) {
                return std::invoke(std::forward<F>(func), Value());
            }
            using ResultType = std::invoke_result_t<F, const T&>;
            return ResultType::Failure(GetError());
        }

        template<typename F>
        constexpr auto FlatMap(F&& func) && -> std::invoke_result_t<F, T&&> {
            if (HasValue()) {
                return std::invoke(std::forward<F>(func), std::move(*this).Value());
            }
            using ResultType = std::invoke_result_t<F, T&&>;
            return ResultType::Failure(std::move(*this).GetError());
        }

        template<typename F>
        constexpr Expected MapError(F&& func) const& {
            if (HasError()) {
                return Failure(std::invoke(std::forward<F>(func), GetError()));
            }
            return Success(Value());
        }

        template<typename F>
        constexpr const Expected& OnError(F&& func) const& {
            if (HasError()) {
                std::invoke(std::forward<F>(func), GetError());
            }
            return *this;
        }

        constexpr explicit operator bool() const noexcept {
            return HasValue();
        }

    private:
        Expected() = default;
        std::variant<T, E> data_;
    };

}
