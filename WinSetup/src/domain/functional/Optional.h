#pragma once

#include <optional>
#include <functional>
#include <type_traits>
#include <stdexcept>

namespace winsetup::domain {

    template<typename T>
    class Optional {
    public:
        Optional() = default;

        Optional(std::nullopt_t) noexcept
            : value_(std::nullopt) {
        }

        Optional(T value)
            : value_(std::move(value)) {
        }

        Optional(const std::optional<T>& opt)
            : value_(opt) {
        }

        Optional(std::optional<T>&& opt) noexcept
            : value_(std::move(opt)) {
        }

        Optional(const Optional&) = default;
        Optional(Optional&&) noexcept = default;
        Optional& operator=(const Optional&) = default;
        Optional& operator=(Optional&&) noexcept = default;

        [[nodiscard]] constexpr bool HasValue() const noexcept {
            return value_.has_value();
        }

        [[nodiscard]] const T& Value() const& {
            if (!HasValue()) [[unlikely]] {
                throw std::logic_error("Optional does not contain a value");
            }
            return *value_;
        }

        [[nodiscard]] T& Value()& {
            if (!HasValue()) [[unlikely]] {
                throw std::logic_error("Optional does not contain a value");
            }
            return *value_;
        }

        [[nodiscard]] T&& Value()&& {
            if (!HasValue()) [[unlikely]] {
                throw std::logic_error("Optional does not contain a value");
            }
            return std::move(*value_);
        }

        [[nodiscard]] constexpr T ValueOr(T defaultValue) const& {
            return HasValue() ? *value_ : std::move(defaultValue);
        }

        [[nodiscard]] constexpr T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(*value_) : std::move(defaultValue);
        }

        template<typename F>
        [[nodiscard]] constexpr auto Map(F&& func) const&
            -> Optional<std::invoke_result_t<F, const T&>> {
            using U = std::invoke_result_t<F, const T&>;
            if (HasValue()) [[likely]] {
                return Optional<U>(std::invoke(std::forward<F>(func), *value_));
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] constexpr auto Map(F&& func) &&
            -> Optional<std::invoke_result_t<F, T&&>> {
            using U = std::invoke_result_t<F, T&&>;
            if (HasValue()) [[likely]] {
                return Optional<U>(std::invoke(std::forward<F>(func), std::move(*value_)));
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] constexpr auto FlatMap(F&& func) const&
            -> std::invoke_result_t<F, const T&> {
            if (HasValue()) [[likely]] {
                return std::invoke(std::forward<F>(func), *value_);
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] constexpr auto FlatMap(F&& func) &&
            -> std::invoke_result_t<F, T&&> {
            if (HasValue()) [[likely]] {
                return std::invoke(std::forward<F>(func), std::move(*value_));
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] constexpr Optional Filter(F&& predicate) const& {
            if (HasValue() && std::invoke(std::forward<F>(predicate), *value_)) [[likely]] {
                return *this;
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] constexpr Optional Filter(F&& predicate)&& {
            if (HasValue() && std::invoke(std::forward<F>(predicate), *value_)) [[likely]] {
                return std::move(*this);
            }
            return std::nullopt;
        }

        template<typename F>
        constexpr const Optional& OnValue(F&& func) const& {
            if (HasValue()) [[likely]] {
                std::invoke(std::forward<F>(func), *value_);
            }
            return *this;
        }

        template<typename F>
        constexpr Optional& OnValue(F&& func)& {
            if (HasValue()) [[likely]] {
                std::invoke(std::forward<F>(func), *value_);
            }
            return *this;
        }

        template<typename F>
        constexpr Optional&& OnValue(F&& func)&& {
            if (HasValue()) [[likely]] {
                std::invoke(std::forward<F>(func), std::move(*value_));
            }
            return std::move(*this);
        }

        template<typename F>
        constexpr const Optional& OnEmpty(F&& func) const& {
            if (!HasValue()) [[unlikely]] {
                std::invoke(std::forward<F>(func));
            }
            return *this;
        }

        template<typename F>
        constexpr Optional& OnEmpty(F&& func)& {
            if (!HasValue()) [[unlikely]] {
                std::invoke(std::forward<F>(func));
            }
            return *this;
        }

        template<typename F>
        constexpr Optional&& OnEmpty(F&& func)&& {
            if (!HasValue()) [[unlikely]] {
                std::invoke(std::forward<F>(func));
            }
            return std::move(*this);
        }

        constexpr void Reset() noexcept {
            value_.reset();
        }

        [[nodiscard]] constexpr const T* operator->() const {
            return &(*value_);
        }

        [[nodiscard]] constexpr T* operator->() {
            return &(*value_);
        }

        [[nodiscard]] constexpr const T& operator*() const& {
            return *value_;
        }

        [[nodiscard]] constexpr T& operator*()& {
            return *value_;
        }

        [[nodiscard]] constexpr T&& operator*()&& {
            return std::move(*value_);
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return HasValue();
        }

        [[nodiscard]] constexpr const std::optional<T>& GetStdOptional() const noexcept {
            return value_;
        }

        [[nodiscard]] constexpr std::optional<T>& GetStdOptional() noexcept {
            return value_;
        }

    private:
        std::optional<T> value_;
    };

    template<typename T>
    [[nodiscard]] constexpr bool operator==(const Optional<T>& lhs, const Optional<T>& rhs) {
        if (lhs.HasValue() != rhs.HasValue()) {
            return false;
        }
        if (!lhs.HasValue()) {
            return true;
        }
        return *lhs == *rhs;
    }

    template<typename T>
    [[nodiscard]] constexpr bool operator!=(const Optional<T>& lhs, const Optional<T>& rhs) {
        return !(lhs == rhs);
    }

    template<typename T>
    [[nodiscard]] constexpr bool operator==(const Optional<T>& opt, std::nullopt_t) noexcept {
        return !opt.HasValue();
    }

    template<typename T>
    [[nodiscard]] constexpr bool operator==(std::nullopt_t, const Optional<T>& opt) noexcept {
        return !opt.HasValue();
    }

    template<typename T>
    [[nodiscard]] constexpr bool operator!=(const Optional<T>& opt, std::nullopt_t) noexcept {
        return opt.HasValue();
    }

    template<typename T>
    [[nodiscard]] constexpr bool operator!=(std::nullopt_t, const Optional<T>& opt) noexcept {
        return opt.HasValue();
    }

    template<typename T>
    [[nodiscard]] constexpr Optional<std::decay_t<T>> MakeOptional(T&& value) {
        return Optional<std::decay_t<T>>(std::forward<T>(value));
    }

}
