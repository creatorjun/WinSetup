// src/domain/functional/Optional.h

#pragma once

#include <optional>
#include <functional>
#include <stdexcept>

namespace winsetup::domain {

    template<typename T>
    class Optional {
    public:
        Optional() = default;
        Optional(std::nullopt_t) : value_(std::nullopt) {}
        Optional(T value) : value_(std::move(value)) {}

        Optional(const Optional&) = default;
        Optional(Optional&&) noexcept = default;
        Optional& operator=(const Optional&) = default;
        Optional& operator=(Optional&&) noexcept = default;

        bool HasValue() const noexcept {
            return value_.has_value();
        }

        const T& Value() const& {
            if (!HasValue()) {
                throw std::logic_error("Optional does not contain a value");
            }
            return *value_;
        }

        T& Value()& {
            if (!HasValue()) {
                throw std::logic_error("Optional does not contain a value");
            }
            return *value_;
        }

        T&& Value()&& {
            if (!HasValue()) {
                throw std::logic_error("Optional does not contain a value");
            }
            return std::move(*value_);
        }

        T ValueOr(T defaultValue) const& {
            return HasValue() ? *value_ : std::move(defaultValue);
        }

        T ValueOr(T defaultValue)&& {
            return HasValue() ? std::move(*value_) : std::move(defaultValue);
        }

        template<typename F>
        auto Map(F&& func) const& -> Optional<decltype(func(std::declval<T>()))> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Optional<U>(func(*value_));
            }
            return std::nullopt;
        }

        template<typename F>
        auto Map(F&& func) && -> Optional<decltype(func(std::declval<T>()))> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Optional<U>(func(std::move(*value_)));
            }
            return std::nullopt;
        }

        template<typename F>
        auto FlatMap(F&& func) const& -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(*value_);
            }
            return std::nullopt;
        }

        template<typename F>
        auto FlatMap(F&& func) && -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(std::move(*value_));
            }
            return std::nullopt;
        }

        template<typename F>
        Optional Filter(F&& predicate) const& {
            if (HasValue() && predicate(*value_)) {
                return *this;
            }
            return std::nullopt;
        }

        template<typename F>
        Optional OrElse(F&& func) const {
            if (HasValue()) {
                return *this;
            }
            return func();
        }

        explicit operator bool() const noexcept {
            return HasValue();
        }

    private:
        std::optional<T> value_;
    };

}
