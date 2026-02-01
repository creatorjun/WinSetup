// src/domain/primitives/Result.h

#pragma once

#include "Error.h"
#include <optional>
#include <stdexcept>

namespace winsetup::domain {

    template<typename E = Error>
    class Result {
    public:
        Result(const Result&) = default;
        Result(Result&&) noexcept = default;
        Result& operator=(const Result&) = default;
        Result& operator=(Result&&) noexcept = default;

        static Result Success() {
            return Result();
        }

        static Result Failure(E error) {
            return Result(std::move(error));
        }

        bool IsSuccess() const noexcept {
            return !error_.has_value();
        }

        bool IsFailure() const noexcept {
            return error_.has_value();
        }

        const E& GetError() const& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return *error_;
        }

        E& GetError()& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return *error_;
        }

        E&& GetError()&& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return std::move(*error_);
        }

        template<typename F>
        Result Then(F&& func) const {
            if (IsSuccess()) {
                return func();
            }
            return Result::Failure(GetError());
        }

        template<typename F>
        Result OnError(F&& func) const {
            if (IsFailure()) {
                func(GetError());
            }
            return *this;
        }

        explicit operator bool() const noexcept {
            return IsSuccess();
        }

    private:
        Result() = default;
        explicit Result(E error) : error_(std::move(error)) {}

        std::optional<E> error_;
    };

    template<>
    class Result<void> {
    public:
        Result(const Result&) = default;
        Result(Result&&) noexcept = default;
        Result& operator=(const Result&) = default;
        Result& operator=(Result&&) noexcept = default;

        static Result Success() {
            return Result();
        }

        static Result Failure(Error error) {
            return Result(std::move(error));
        }

        bool IsSuccess() const noexcept {
            return !error_.has_value();
        }

        bool IsFailure() const noexcept {
            return error_.has_value();
        }

        const Error& GetError() const& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return *error_;
        }

        Error& GetError()& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return *error_;
        }

        Error&& GetError()&& {
            if (!error_.has_value()) {
                throw std::logic_error("Result does not contain an error");
            }
            return std::move(*error_);
        }

        template<typename F>
        Result Then(F&& func) const {
            if (IsSuccess()) {
                return func();
            }
            return Result::Failure(GetError());
        }

        template<typename F>
        Result OnError(F&& func) const {
            if (IsFailure()) {
                func(GetError());
            }
            return *this;
        }

        explicit operator bool() const noexcept {
            return IsSuccess();
        }

    private:
        Result() = default;
        explicit Result(Error err) : error_(std::move(err)) {}

        std::optional<Error> error_;
    };

}
