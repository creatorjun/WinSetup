// src/domain/functional/Pipeline.h

#pragma once

#include <utility>
#include <functional>

namespace winsetup::domain {

    template<typename T>
    class Pipeline {
    public:
        explicit Pipeline(T value) : m_value(std::move(value)) {}

        template<typename F>
        auto Then(F&& func) -> Pipeline<std::invoke_result_t<F, T>> {
            using ResultType = std::invoke_result_t<F, T>;
            return Pipeline<ResultType>(func(std::move(m_value)));
        }

        template<typename F>
        auto ThenIf(bool condition, F&& func) -> Pipeline<T> {
            if (condition) {
                m_value = func(std::move(m_value));
            }
            return std::move(*this);
        }

        template<typename F>
        Pipeline<T>& Tap(F&& func) {
            func(m_value);
            return *this;
        }

        T Unwrap()&& {
            return std::move(m_value);
        }

        const T& Unwrap() const& {
            return m_value;
        }

    private:
        T m_value;
    };

    template<typename T>
    Pipeline<T> MakePipeline(T value) {
        return Pipeline<T>(std::move(value));
    }

}
