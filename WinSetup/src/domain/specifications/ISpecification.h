// src/domain/specifications/ISpecification.h

#pragma once

#include <memory>

namespace winsetup::domain {

    template<typename T>
    class ISpecification {
    public:
        virtual ~ISpecification() = default;
        virtual bool IsSatisfiedBy(const T& entity) const = 0;
        virtual std::shared_ptr<ISpecification<T>> Clone() const = 0;

        std::shared_ptr<ISpecification<T>> And(std::shared_ptr<ISpecification<T>> other) const;
        std::shared_ptr<ISpecification<T>> Or(std::shared_ptr<ISpecification<T>> other) const;
        std::shared_ptr<ISpecification<T>> Not() const;
    };

    template<typename T>
    class AndSpecification : public ISpecification<T> {
    public:
        AndSpecification(
            std::shared_ptr<ISpecification<T>> left,
            std::shared_ptr<ISpecification<T>> right)
            : m_left(std::move(left))
            , m_right(std::move(right))
        {
        }

        bool IsSatisfiedBy(const T& entity) const override {
            return m_left->IsSatisfiedBy(entity) && m_right->IsSatisfiedBy(entity);
        }

        std::shared_ptr<ISpecification<T>> Clone() const override {
            return std::make_shared<AndSpecification<T>>(m_left->Clone(), m_right->Clone());
        }

    private:
        std::shared_ptr<ISpecification<T>> m_left;
        std::shared_ptr<ISpecification<T>> m_right;
    };

    template<typename T>
    class OrSpecification : public ISpecification<T> {
    public:
        OrSpecification(
            std::shared_ptr<ISpecification<T>> left,
            std::shared_ptr<ISpecification<T>> right)
            : m_left(std::move(left))
            , m_right(std::move(right))
        {
        }

        bool IsSatisfiedBy(const T& entity) const override {
            return m_left->IsSatisfiedBy(entity) || m_right->IsSatisfiedBy(entity);
        }

        std::shared_ptr<ISpecification<T>> Clone() const override {
            return std::make_shared<OrSpecification<T>>(m_left->Clone(), m_right->Clone());
        }

    private:
        std::shared_ptr<ISpecification<T>> m_left;
        std::shared_ptr<ISpecification<T>> m_right;
    };

    template<typename T>
    class NotSpecification : public ISpecification<T> {
    public:
        explicit NotSpecification(std::shared_ptr<ISpecification<T>> spec)
            : m_spec(std::move(spec))
        {
        }

        bool IsSatisfiedBy(const T& entity) const override {
            return !m_spec->IsSatisfiedBy(entity);
        }

        std::shared_ptr<ISpecification<T>> Clone() const override {
            return std::make_shared<NotSpecification<T>>(m_spec->Clone());
        }

    private:
        std::shared_ptr<ISpecification<T>> m_spec;
    };

    template<typename T>
    std::shared_ptr<ISpecification<T>> ISpecification<T>::And(
        std::shared_ptr<ISpecification<T>> other) const
    {
        return std::make_shared<AndSpecification<T>>(
            this->Clone(),
            std::move(other)
        );
    }

    template<typename T>
    std::shared_ptr<ISpecification<T>> ISpecification<T>::Or(
        std::shared_ptr<ISpecification<T>> other) const
    {
        return std::make_shared<OrSpecification<T>>(
            this->Clone(),
            std::move(other)
        );
    }

    template<typename T>
    std::shared_ptr<ISpecification<T>> ISpecification<T>::Not() const {
        return std::make_shared<NotSpecification<T>>(this->Clone());
    }

}
