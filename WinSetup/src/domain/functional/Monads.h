// src/domain/functional/Monads.h

#pragma once

#include <domain/primitives/Expected.h>
#include <functional>
#include <optional>
#include <type_traits>

namespace winsetup::domain {

    template<typename T, typename F>
    auto Map(const Expected<T>& exp, F&& func) -> Expected<std::invoke_result_t<F, const T&>> {
        using ReturnType = std::invoke_result_t<F, const T&>;

        if (!exp.HasValue()) {
            return Expected<ReturnType>::FromError(exp.GetError());
        }

        return Expected<ReturnType>(func(exp.GetValue()));
    }

    template<typename T, typename F>
    auto FlatMap(const Expected<T>& exp, F&& func) -> std::invoke_result_t<F, const T&> {
        if (!exp.HasValue()) {
            using ReturnType = std::invoke_result_t<F, const T&>;
            return ReturnType::FromError(exp.GetError());
        }

        return func(exp.GetValue());
    }

    template<typename T>
    T UnwrapOr(const Expected<T>& exp, T defaultValue) {
        return exp.HasValue() ? exp.GetValue() : defaultValue;
    }

    template<typename T, typename F>
    auto UnwrapOrElse(const Expected<T>& exp, F&& func) -> T {
        return exp.HasValue() ? exp.GetValue() : func(exp.GetError());
    }

    template<typename T, typename F>
    Expected<T> Filter(const Expected<T>& exp, F&& predicate, const Error& error) {
        if (!exp.HasValue()) {
            return exp;
        }

        if (predicate(exp.GetValue())) {
            return exp;
        }

        return Expected<T>::FromError(error);
    }

    template<typename T, typename U>
    Expected<std::pair<T, U>> Zip(const Expected<T>& first, const Expected<U>& second) {
        if (!first.HasValue()) {
            return Expected<std::pair<T, U>>::FromError(first.GetError());
        }

        if (!second.HasValue()) {
            return Expected<std::pair<T, U>>::FromError(second.GetError());
        }

        return Expected<std::pair<T, U>>(std::make_pair(first.GetValue(), second.GetValue()));
    }

}
