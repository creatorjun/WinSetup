// src/domain/specifications/DiskSpecifications.h

#pragma once

#include <domain/specifications/ISpecification.h>
#include <domain/entities/DiskInfo.h>
#include <domain/valueobjects/DiskSize.h>

namespace winsetup::domain {

    class DiskHasMinimumSizeSpec : public ISpecification<DiskInfo> {
    public:
        explicit DiskHasMinimumSizeSpec(DiskSize minSize) : m_minSize(minSize) {}

        bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.GetSize() >= m_minSize;
        }

        std::shared_ptr<ISpecification<DiskInfo>> Clone() const override {
            return std::make_shared<DiskHasMinimumSizeSpec>(m_minSize);
        }

    private:
        DiskSize m_minSize;
    };

    class DiskIsSSDSpec : public ISpecification<DiskInfo> {
    public:
        bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.IsSSD();
        }

        std::shared_ptr<ISpecification<DiskInfo>> Clone() const override {
            return std::make_shared<DiskIsSSDSpec>();
        }
    };

    class DiskIsHDDSpec : public ISpecification<DiskInfo> {
    public:
        bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.IsHDD();
        }

        std::shared_ptr<ISpecification<DiskInfo>> Clone() const override {
            return std::make_shared<DiskIsHDDSpec>();
        }
    };

    class DiskIsValidSpec : public ISpecification<DiskInfo> {
    public:
        bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.IsValid();
        }

        std::shared_ptr<ISpecification<DiskInfo>> Clone() const override {
            return std::make_shared<DiskIsValidSpec>();
        }
    };

    class DiskHasEnoughSpaceSpec : public ISpecification<DiskInfo> {
    public:
        explicit DiskHasEnoughSpaceSpec(DiskSize required) : m_required(required) {}

        bool IsSatisfiedBy(const DiskInfo& disk) const override {
            return disk.HasEnoughSpace(m_required);
        }

        std::shared_ptr<ISpecification<DiskInfo>> Clone() const override {
            return std::make_shared<DiskHasEnoughSpaceSpec>(m_required);
        }

    private:
        DiskSize m_required;
    };

}
