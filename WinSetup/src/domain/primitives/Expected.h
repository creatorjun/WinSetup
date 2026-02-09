// src/domain/primitives/Expected.h
#pragma once

#include <utility>
#include <type_traits>
#include <optional>
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

        [[nodiscard]] T& Value() & noexcept {
            return m_value;
        }

        [[nodiscard]] const T& Value() const& noexcept {
            return m_value;
        }

        [[nodiscard]] T&& Value() && noexcept {
            return std::move(m_value);
        }

        [[nodiscard]] const Error& GetError() const noexcept {
            return m_error;
        }

        [[nodiscard]] std::optional<T> ToOptional() const {
            if (m_hasValue) {
                return m_value;
            }
            return std::nullopt;
        }

        template<typename F>
        [[nodiscard]] auto Map(F&& func) -> Expected<std::invoke_result_t<F, T>> {
            using U = std::invoke_result_t<F, T>;

            if (m_hasValue) {
                return Expected<U>(func(m_value));
            }
            else {
                return Expected<U>(m_error);
            }
        }

        template<typename F>
        [[nodiscard]] auto FlatMap(F&& func) -> std::invoke_result_t<F, T> {
            using Result = std::invoke_result_t<F, T>;

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

        template<typename F>
        Expected& OnSuccess(F&& func)& {
            if (m_hasValue) {
                func(m_value);
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
        Expected()
            : m_hasValue(true)
        {
        }

        Expected(Error error)
            : m_hasValue(false)
        {
            new (&m_storage) Error(std::move(error));
        }

        ~Expected() {
            if (!m_hasValue) {
                reinterpret_cast<Error*>(&m_storage)->~Error();
            }
        }

        Expected(const Expected& other)
            : m_hasValue(other.m_hasValue)
        {
            if (!m_hasValue) {
                new (&m_storage) Error(*reinterpret_cast<const Error*>(&other.m_storage));
            }
        }

        Expected(Expected&& other) noexcept
            : m_hasValue(other.m_hasValue)
        {
            if (!m_hasValue) {
                new (&m_storage) Error(std::move(*reinterpret_cast<Error*>(&other.m_storage)));
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

        [[nodiscard]] const Error& GetError() const noexcept {
            return *reinterpret_cast<const Error*>(&m_storage);
        }

        template<typename F>
        Expected& OnError(F&& func)& {
            if (!m_hasValue) {
                func(*reinterpret_cast<Error*>(&m_storage));
            }
            return *this;
        }

        template<typename F>
        Expected& OnSuccess(F&& func)& {
            if (m_hasValue) {
                func();
            }
            return *this;
        }

    private:
        bool m_hasValue;
        alignas(Error) unsigned char m_storage[sizeof(Error)];
    };

}
