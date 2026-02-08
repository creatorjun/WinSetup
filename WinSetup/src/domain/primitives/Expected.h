// src/domain/primitives/Expected.h
#pragma once

#include <variant>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "Error.h"

namespace winsetup::domain {

    template<typename T, typename E = Error>
    class Expected {
    public:
        Expected(const T& value) : m_data(value) {}
        Expected(T&& value) noexcept : m_data(std::move(value)) {}
        Expected(const E& error) : m_data(error) {}
        Expected(E&& error) noexcept : m_data(std::move(error)) {}

        Expected(const Expected&) = default;
        Expected(Expected&&) noexcept = default;
        Expected& operator=(const Expected&) = default;
        Expected& operator=(Expected&&) noexcept = default;

        [[nodiscard]] bool HasValue() const noexcept {
            return std::holds_alternative<T>(m_data);
        }

        [[nodiscard]] bool HasError() const noexcept {
            return std::holds_alternative<E>(m_data);
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return HasValue();
        }

        [[nodiscard]] const T& Value() const& {
            if (HasError()) {
                throw std::runtime_error("Expected contains error");
            }
            return std::get<T>(m_data);
        }

        [[nodiscard]] T& Value()& {
            if (HasError()) {
                throw std::runtime_error("Expected contains error");
            }
            return std::get<T>(m_data);
        }

        [[nodiscard]] T&& Value()&& {
            if (HasError()) {
                throw std::runtime_error("Expected contains error");
            }
            return std::move(std::get<T>(m_data));
        }

        [[nodiscard]] const E& Error() const& {
            if (HasValue()) {
                throw std::runtime_error("Expected contains value");
            }
            return std::get<E>(m_data);
        }

        [[nodiscard]] E& Error()& {
            if (HasValue()) {
                throw std::runtime_error("Expected contains value");
            }
            return std::get<E>(m_data);
        }

        [[nodiscard]] E&& Error()&& {
            if (HasValue()) {
                throw std::runtime_error("Expected contains value");
            }
            return std::move(std::get<E>(m_data));
        }

        [[nodiscard]] const T& ValueOr(const T& defaultValue) const& noexcept {
            return HasValue() ? std::get<T>(m_data) : defaultValue;
        }

        [[nodiscard]] T ValueOr(T&& defaultValue) && noexcept {
            return HasValue() ? std::move(std::get<T>(m_data)) : std::move(defaultValue);
        }

        template<typename Func>
        [[nodiscard]] auto Map(Func&& func) const& -> Expected<decltype(func(std::declval<T>())), E> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Expected<U, E>(func(std::get<T>(m_data)));
            }
            return Expected<U, E>(std::get<E>(m_data));
        }

        template<typename Func>
        [[nodiscard]] auto Map(Func&& func) && -> Expected<decltype(func(std::declval<T>())), E> {
            using U = decltype(func(std::declval<T>()));
            if (HasValue()) {
                return Expected<U, E>(func(std::move(std::get<T>(m_data))));
            }
            return Expected<U, E>(std::move(std::get<E>(m_data)));
        }

        template<typename Func>
        [[nodiscard]] auto AndThen(Func&& func) const& -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(std::get<T>(m_data));
            }
            return decltype(func(std::declval<T>()))(std::get<E>(m_data));
        }

        template<typename Func>
        [[nodiscard]] auto AndThen(Func&& func) && -> decltype(func(std::declval<T>())) {
            if (HasValue()) {
                return func(std::move(std::get<T>(m_data)));
            }
            return decltype(func(std::declval<T>()))(std::move(std::get<E>(m_data)));
        }

        template<typename Func>
        [[nodiscard]] Expected OrElse(Func&& func) const& {
            if (HasError()) {
                return func(std::get<E>(m_data));
            }
            return *this;
        }

        template<typename Func>
        [[nodiscard]] Expected OrElse(Func&& func)&& {
            if (HasError()) {
                return func(std::move(std::get<E>(m_data)));
            }
            return std::move(*this);
        }

    private:
        std::variant<T, E> m_data;
    };

}
