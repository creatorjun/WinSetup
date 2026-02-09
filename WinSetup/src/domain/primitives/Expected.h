// src/domain/primitives/Expected.h
#pragma once

#include <utility>
#include <type_traits>
#include <stdexcept>
#include "Error.h"

namespace winsetup::domain {

    template<typename T>
    class Expected {
    public:
        Expected(T value)
            : m_hasValue(true)
        {
            new (&m_value) T(std::move(value));
        }

        Expected(Error error)
            : m_hasValue(false)
        {
            new (&m_error) Error(std::move(error));
        }

        ~Expected() {
            if (m_hasValue) {
                m_value.~T();
            }
            else {
                m_error.~Error();
            }
        }

        Expected(const Expected& other)
            : m_hasValue(other.m_hasValue)
        {
            if (m_hasValue) {
                new (&m_value) T(other.m_value);
            }
            else {
                new (&m_error) Error(other.m_error);
            }
        }

        Expected(Expected&& other) noexcept
            : m_hasValue(other.m_hasValue)
        {
            if (m_hasValue) {
                new (&m_value) T(std::move(other.m_value));
            }
            else {
                new (&m_error) Error(std::move(other.m_error));
            }
        }

        Expected& operator=(const Expected& other) {
            if (this != &other) {
                this->~Expected();
                new (this) Expected(other);
            }
            return *this;
        }

        Expected& operator=(Expected&& other) noexcept {
            if (this != &other) {
                this->~Expected();
                new (this) Expected(std::move(other));
            }
            return *this;
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return m_hasValue;
        }

        [[nodiscard]] T& Value()& {
            if (!m_hasValue) {
                throw std::logic_error("Expected does not contain value");
            }
            return m_value;
        }

        [[nodiscard]] const T& Value() const& {
            if (!m_hasValue) {
                throw std::logic_error("Expected does not contain value");
            }
            return m_value;
        }

        [[nodiscard]] T&& Value()&& {
            if (!m_hasValue) {
                throw std::logic_error("Expected does not contain value");
            }
            return std::move(m_value);
        }

        [[nodiscard]] const Error& GetError() const {
            if (m_hasValue) {
                throw std::logic_error("Expected does not contain error");
            }
            return m_error;
        }

        template<typename F>
        [[nodiscard]] auto Map(F&& func) -> Expected<decltype(func(std::declval<T>()))> {
            using U = decltype(func(std::declval<T>()));

            if (m_hasValue) {
                return Expected<U>(func(m_value));
            }
            else {
                return Expected<U>(m_error);
            }
        }

        template<typename F>
        [[nodiscard]] auto FlatMap(F&& func) -> decltype(func(std::declval<T>())) {
            using Result = decltype(func(std::declval<T>()));

            if (m_hasValue) {
                return func(m_value);
            }
            else {
                return Result(m_error);
            }
        }

        [[nodiscard]] T UnwrapOr(T defaultValue)&& {
            if (m_hasValue) {
                return std::move(m_value);
            }
            else {
                return defaultValue;
            }
        }

        template<typename F>
        [[nodiscard]] T UnwrapOrElse(F&& func)&& {
            if (m_hasValue) {
                return std::move(m_value);
            }
            else {
                return func(m_error);
            }
        }

        template<typename F>
        Expected& OnError(F&& func)& {
            if (!m_hasValue) {
                func(m_error);
            }
            return *this;
        }

    private:
        union {
            T m_value;
            Error m_error;
        };
        bool m_hasValue;
    };

    template<>
    class Expected<void> {
    public:
        Expected() : m_hasValue(true) {}

        Expected(Error error)
            : m_hasValue(false)
            , m_error(std::move(error))
        {
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return m_hasValue;
        }

        [[nodiscard]] const Error& GetError() const {
            if (m_hasValue) {
                throw std::logic_error("Expected does not contain error");
            }
            return m_error;
        }

        template<typename F>
        Expected& OnError(F&& func)& {
            if (!m_hasValue) {
                func(m_error);
            }
            return *this;
        }

    private:
        bool m_hasValue;
        Error m_error;
    };

}
