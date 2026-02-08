// src/domain/functional/Compose.h

#pragma once

#include <functional>
#include <utility>

namespace winsetup::domain {

    template<typename F, typename G>
    auto Compose(F&& f, G&& g) {
        return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&&... args) {
            return f(g(std::forward<decltype(args)>(args)...));
            };
    }

    template<typename F, typename... Fs>
    auto ComposeAll(F&& f, Fs&&... fs) {
        if constexpr (sizeof...(fs) == 0) {
            return std::forward<F>(f);
        }
        else {
            return Compose(std::forward<F>(f), ComposeAll(std::forward<Fs>(fs)...));
        }
    }

    template<typename T>
    auto Identity() {
        return [](T&& value) -> T&& {
            return std::forward<T>(value);
            };
    }

    template<typename F>
    auto Curry(F&& f) {
        return [f = std::forward<F>(f)](auto&&... args1) {
            if constexpr (std::is_invocable_v<F, decltype(args1)...>) {
                return f(std::forward<decltype(args1)>(args1)...);
            }
            else {
                return [f, ...bound = std::forward<decltype(args1)>(args1)](auto&&... args2) {
                    return f(bound..., std::forward<decltype(args2)>(args2)...);
                    };
            }
            };
    }

}
