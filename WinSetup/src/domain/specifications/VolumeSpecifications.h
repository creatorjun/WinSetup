// src/domain/specifications/VolumeSpecifications.h
#pragma once

#include <domain/specifications/ISpecification.h>
#include <domain/entities/VolumeInfo.h>
#include <domain/valueobjects/DiskSize.h>
#include <domain/valueobjects/FileSystemType.h>

namespace winsetup::domain {

    class VolumeIsSystemSpec : public ISpecification<VolumeInfo> {
    public:
        bool IsSatisfiedBy(const VolumeInfo& volume) const override {
            return volume.IsSystem();
        }

        std::shared_ptr<ISpecification<VolumeInfo>> Clone() const override {
            return std::make_shared<VolumeIsSystemSpec>();
        }
    };

    class VolumeIsBootSpec : public ISpecification<VolumeInfo> {
    public:
        bool IsSatisfiedBy(const VolumeInfo& volume) const override {
            return volume.IsBoot();
        }

        std::shared_ptr<ISpecification<VolumeInfo>> Clone() const override {
            return std::make_shared<VolumeIsBootSpec>();
        }
    };

    class VolumeHasMinimumSizeSpec : public ISpecification<VolumeInfo> {
    public:
        explicit VolumeHasMinimumSizeSpec(DiskSize minSize) : m_minSize(minSize) {}

        bool IsSatisfiedBy(const VolumeInfo& volume) const override {
            return volume.GetSize() >= m_minSize;
        }

        std::shared_ptr<ISpecification<VolumeInfo>> Clone() const override {
            return std::make_shared<VolumeHasMinimumSizeSpec>(m_minSize);
        }

    private:
        DiskSize m_minSize;
    };

    class VolumeHasFileSystemSpec : public ISpecification<VolumeInfo> {
    public:
        explicit VolumeHasFileSystemSpec(FileSystemType fs) : m_fileSystem(fs) {}

        bool IsSatisfiedBy(const VolumeInfo& volume) const override {
            return volume.GetFileSystem() == m_fileSystem;
        }

        std::shared_ptr<ISpecification<VolumeInfo>> Clone() const override {
            return std::make_shared<VolumeHasFileSystemSpec>(m_fileSystem);
        }

    private:
        FileSystemType m_fileSystem;
    };

    class VolumeIsValidSpec : public ISpecification<VolumeInfo> {
    public:
        bool IsSatisfiedBy(const VolumeInfo& volume) const override {
            return volume.IsValid();
        }

        std::shared_ptr<ISpecification<VolumeInfo>> Clone() const override {
            return std::make_shared<VolumeIsValidSpec>();
        }
    };

}
