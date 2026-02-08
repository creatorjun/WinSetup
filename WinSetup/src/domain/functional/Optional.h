// src/domain/functional/Optional.h

#pragma once

#include <optional>
#include <functional>
#include <type_traits>

namespace winsetup::domain {

    template<typename T, typename F>
    auto MapOptional(const std::optional<T>& opt, F&& func)
        -> std::optional<std::invoke_result_t<F, const T&>>
    {
        if (!opt.has_value()) {
            return std::nullopt;
        }
        return std::optional(func(*opt));
    }

    template<typename T, typename F>
    auto FlatMapOptional(const std::optional<T>& opt, F&& func)
        -> std::invoke_result_t<F, const T&>
    {
        if (!opt.has_value()) {
            using ReturnType = std::invoke_result_t<F, const T&>;
            return std::nullopt;
        }
        return func(*opt);
    }

    template<typename T>
    T GetOrDefault(const std::optional<T>& opt, T defaultValue) {
        return opt.has_value() ? *opt : defaultValue;
    }

    template<typename T, typename F>
    T GetOrElse(const std::optional<T>& opt, F&& func) {
        return opt.has_value() ? *opt : func();
    }

    template<typename T, typename F>
    std::optional<T> FilterOptional(const std::optional<T>& opt, F&& predicate) {
        if (!opt.has_value()) {
            return std::nullopt;
        }

        if (predicate(*opt)) {
            return opt;
        }

        return std::nullopt;
    }

}
