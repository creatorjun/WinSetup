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

        [[nodiscard]] constexpr bool IsSuccess() const noexcept {
            return std::holds_alternative<SuccessTag>(data_);
        }

        [[nodiscard]] constexpr bool IsFailure() const noexcept {
            return std::holds_alternative<E>(data_);
        }

        const E& GetError() const& {
            if (!IsFailure()) [[unlikely]] {
                throw std::logic_error("Result does not contain an error");
            }
            return std::get<E>(data_);
        }

        E& GetError()& {
            if (!IsFailure()) [[unlikely]] {
                throw std::logic_error("Result does not contain an error");
            }
            return std::get<E>(data_);
        }

        E&& GetError()&& {
            if (!IsFailure()) [[unlikely]] {
                throw std::logic_error("Result does not contain an error");
            }
            return std::move(std::get<E>(data_));
        }

        template<typename F>
        Result Then(F&& func) const& {
            if (IsSuccess()) [[likely]] {
                return std::invoke(std::forward<F>(func));
            }
            return Failure(std::get<E>(data_));
        }

        template<typename F>
        Result Then(F&& func)&& {
            if (IsSuccess()) [[likely]] {
                return std::invoke(std::forward<F>(func));
            }
            return Failure(std::move(std::get<E>(data_)));
        }

        template<typename F>
        const Result& OnError(F&& func) const& {
            if (IsFailure()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Result& OnError(F&& func)& {
            if (IsFailure()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::get<E>(data_));
            }
            return *this;
        }

        template<typename F>
        Result&& OnError(F&& func)&& {
            if (IsFailure()) [[unlikely]] {
                std::invoke(std::forward<F>(func), std::move(std::get<E>(data_)));
            }
            return std::move(*this);
        }

        template<typename F>
        constexpr auto MapError(F&& func) const& -> Result<std::invoke_result_t<F, const E&>> {
            using NewError = std::invoke_result_t<F, const E&>;
            if (IsFailure()) [[unlikely]] {
                return Result<NewError>::Failure(std::invoke(std::forward<F>(func), std::get<E>(data_)));
            }
            return Result<NewError>::Success();
        }

        template<typename F>
        constexpr auto MapError(F&& func) && -> Result<std::invoke_result_t<F, E&&>> {
            using NewError = std::invoke_result_t<F, E&&>;
            if (IsFailure()) [[unlikely]] {
                return Result<NewError>::Failure(std::invoke(std::forward<F>(func), std::move(std::get<E>(data_))));
            }
            return Result<NewError>::Success();
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept {
            return IsSuccess();
        }

    private:
        Result() = default;
        std::variant<SuccessTag, E> data_;
    };

}
