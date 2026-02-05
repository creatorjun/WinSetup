#pragma once

#include "Error.h"
#include <variant>
#include <functional>
#include <type_traits>

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

        [[nodiscard]] constexpr bool HasValue() const noexcept {
            return std::holds_alternative<T>(data_);
        }

        [[nodiscard]] constexpr bool HasError() const noexcept {
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

        template<typename F>
        constexpr auto Map(F&& func) const& -> Expected<std::invoke_result_t<F, const T&>, E> {
            using U = std::invoke_result_t<F, const T&>;
            if (HasValue()) [[likely]] {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), std::get<T>(data_)));
            }
            return Expected<U, E>::Failure(std::get<E>(data_));
        }

        template<typename F>
        constexpr auto Map(F&& func) && -> Expected<std::invoke_result_t<F, T&&>, E> {
            using U = std::invoke_result_t<F, T&&>;
            if (HasValue()) [[likely]] {
                return Expected<U, E>::Success(std::invoke(std::forward<F>(func), std::move(std::get<T>(data_))));
            }
            return Expected<U, E>::Failure(std::move(std::get<E>(data_)));
        }

        template<typename F>
        constexpr auto FlatMap(F&& func) const& -> std::invoke_result_t<F, const T&> {
            if (HasValue()) [[likely]] {
                return std::invoke(std::forward<F>(func), std::get<T>(data_));
            }
            using ResultType = std::invoke_result_t<F, const T&>;
            return ResultType::Failure(std::get<E>(data_));
        }

        template<typename F>
        constexpr auto FlatMap(F&& func) && -> std::invoke_result_t<F, T&&> {
            if (HasValue()) [[likely]] {
                return std::invoke(std::forward<F>(func), std::move(std::get<T>(data_)));
            }
            using ResultType = std::invoke_result_t<F, T&&>;
            return ResultType::Failure(std::move(std::get<E>(data_)));
        }

        template<typename F>
        const Expected& OnError(F&& func) const& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Expected& OnError(F&& func)& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Expected&& OnError(F&& func)&& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::move(std::get<E>(data_)));
            }
            return std::move(*this);
        }

        constexpr T ValueOr(T defaultValue) const& {
            return HasValue() ? std::get<T>(data_) : std::move(defaultValue);
        }

        constexpr T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(std::get<T>(data_)) : std::move(defaultValue);
        }

        template<typename F>
        constexpr auto MapError(F&& func) const& -> Expected<T, std::invoke_result_t<F, const E&>> {
            using NewError = std::invoke_result_t<F, const E&>;
            if (HasError()) [[unlikely]] {
                return Expected<T, NewError>::Failure(std::invoke(std::forward<F>(func), std::get<E>(data_)));
            }
            return Expected<T, NewError>::Success(std::get<T>(data_));
        }

        template<typename F>
        constexpr auto MapError(F&& func) && -> Expected<T, std::invoke_result_t<F, E&&>> {
            using NewError = std::invoke_result_t<F, E&&>;
            if (HasError()) [[unlikely]] {
                return Expected<T, NewError>::Failure(std::invoke(std::forward<F>(func), std::move(std::get<E>(data_))));
            }
            return Expected<T, NewError>::Success(std::move(std::get<T>(data_)));
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept {
            return HasValue();
        }

    private:
        Expected() = default;
        std::variant<T, E> data_;
    };

    template<typename E>
    class Expected<void, E> {
    public:
        Expected(const Expected&) = default;
        Expected(Expected&&) noexcept = default;
        Expected& operator=(const Expected&) = default;
        Expected& operator=(Expected&&) noexcept = default;

        static Expected Success() {
            Expected result;
            result.data_ = SuccessTag{};
            return result;
        }

        static Expected Failure(E error) {
            Expected result;
            result.data_ = std::move(error);
            return result;
        }

        [[nodiscard]] constexpr bool HasValue() const noexcept {
            return std::holds_alternative<SuccessTag>(data_);
        }

        [[nodiscard]] constexpr bool HasError() const noexcept {
            return std::holds_alternative<E>(data_);
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

        template<typename F>
        const Expected& OnError(F&& func) const& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Expected& OnError(F&& func)& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Expected&& OnError(F&& func)&& {
            if (HasError()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::move(std::get<E>(data_)));
            }
            return std::move(*this);
        }

        template<typename F>
        constexpr auto MapError(F&& func) const& -> Expected<void, std::invoke_result_t<F, const E&>> {
            using NewError = std::invoke_result_t<F, const E&>;
            if (HasError()) [[unlikely]] {
                return Expected<void, NewError>::Failure(std::invoke(std::forward<F>(func), std::get<E>(data_)));
            }
            return Expected<void, NewError>::Success();
        }

        template<typename F>
        constexpr auto MapError(F&& func) && -> Expected<void, std::invoke_result_t<F, E&&>> {
            using NewError = std::invoke_result_t<F, E&&>;
            if (HasError()) [[unlikely]] {
                return Expected<void, NewError>::Failure(std::invoke(std::forward<F>(func), std::move(std::get<E>(data_))));
            }
            return Expected<void, NewError>::Success();
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept {
            return HasValue();
        }

    private:
        struct SuccessTag {};
        Expected() = default;
        std::variant<SuccessTag, E> data_;
    };

}
