#pragma once

#include "Error.h"
#include <variant>
#include <functional>

namespace winsetup::domain {

    struct SuccessTag {};
    constexpr SuccessTag Success{};

    template<typename E = Error>
    class Result {
    public:
        Result(const Result&) = default;
        Result(Result&&) noexcept = default;
        Result& operator=(const Result&) = default;
        Result& operator=(Result&&) noexcept = default;

        static Result Success() {
            Result result;
            result.data_ = SuccessTag{};
            return result;
        }

        static Result Failure(E error) {
            Result result;
            result.data_ = std::move(error);
            return result;
        }

        bool IsSuccess() const noexcept {
            return std::holds_alternative<SuccessTag>(data_);
        }

        bool IsFailure() const noexcept {
            return std::holds_alternative<E>(data_);
        }

        const E& GetError() const& {
            if (!IsFailure()) {
                throw std::logic_error("Result does not contain an error");
            }
            return std::get<E>(data_);
        }

        E& GetError()& {
            if (!IsFailure()) {
                throw std::logic_error("Result does not contain an error");
            }
            return std::get<E>(data_);
        }

        E&& GetError()&& {
            if (!IsFailure()) {
                throw std::logic_error("Result does not contain an error");
            }
            return std::move(std::get<E>(data_));
        }

        template<typename F>
        Result Then(F&& func) const {
            if (IsSuccess()) {
                return func();
            }
            return Failure(GetError());
        }

        template<typename F>
        const Result& OnError(F&& func) const {
            if (IsFailure()) {
                func(GetError());
            }
            return *this;
        }

        template<typename F>
        auto MapError(F&& func) const -> Result<decltype(func(std::declval<E>()))> {
            using NewError = decltype(func(std::declval<E>()));
            if (IsFailure()) {
                return Result<NewError>::Failure(func(GetError()));
            }
            return Result<NewError>::Success();
        }

        explicit operator bool() const noexcept {
            return IsSuccess();
        }

    private:
        Result() = default;
        std::variant<SuccessTag, E> data_;
    };

}
