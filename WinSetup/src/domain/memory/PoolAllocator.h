// src/domain/memory/PoolAllocator.h

#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <new>

namespace winsetup::domain {

    template<typename T>
    class PoolAllocator {
    public:
        explicit PoolAllocator(size_t poolSize)
            : poolSize_(poolSize)
            , allocatedCount_(0) {

            if (poolSize == 0) {
                throw std::invalid_argument("Pool size must be greater than 0");
            }

            pool_.reserve(poolSize);
            freeList_.reserve(poolSize);

            buffer_ = static_cast<T*>(::operator new(poolSize * sizeof(T)));

            for (size_t i = 0; i < poolSize; ++i) {
                freeList_.push_back(&buffer_[i]);
            }
        }

        ~PoolAllocator() {
            Clear();
            ::operator delete(buffer_);
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        PoolAllocator(PoolAllocator&& other) noexcept
            : buffer_(other.buffer_)
            , pool_(std::move(other.pool_))
            , freeList_(std::move(other.freeList_))
            , poolSize_(other.poolSize_)
            , allocatedCount_(other.allocatedCount_) {
            other.buffer_ = nullptr;
            other.poolSize_ = 0;
            other.allocatedCount_ = 0;
        }

        PoolAllocator& operator=(PoolAllocator&& other) noexcept {
            if (this != &other) {
                Clear();
                ::operator delete(buffer_);

                buffer_ = other.buffer_;
                pool_ = std::move(other.pool_);
                freeList_ = std::move(other.freeList_);
                poolSize_ = other.poolSize_;
                allocatedCount_ = other.allocatedCount_;

                other.buffer_ = nullptr;
                other.poolSize_ = 0;
                other.allocatedCount_ = 0;
            }
            return *this;
        }

        T* Allocate() {
            if (freeList_.empty()) {
                throw std::bad_alloc();
            }

            T* ptr = freeList_.back();
            freeList_.pop_back();
            pool_.push_back(ptr);
            ++allocatedCount_;

            return ptr;
        }

        void Deallocate(T* ptr) {
            if (!ptr) {
                return;
            }

            auto it = std::find(pool_.begin(), pool_.end(), ptr);
            if (it == pool_.end()) {
                throw std::invalid_argument("Pointer not allocated from this pool");
            }

            pool_.erase(it);
            freeList_.push_back(ptr);
            --allocatedCount_;
        }

        size_t GetPoolSize() const noexcept {
            return poolSize_;
        }

        size_t GetAvailableCount() const noexcept {
            return freeList_.size();
        }

        size_t GetAllocatedCount() const noexcept {
            return allocatedCount_;
        }

        void Clear() {
            pool_.clear();
            freeList_.clear();
            freeList_.reserve(poolSize_);

            for (size_t i = 0; i < poolSize_; ++i) {
                freeList_.push_back(&buffer_[i]);
            }
            allocatedCount_ = 0;
        }

        bool Contains(const T* ptr) const noexcept {
            return ptr >= buffer_ && ptr < buffer_ + poolSize_;
        }

    private:
        T* buffer_;
        std::vector<T*> pool_;
        std::vector<T*> freeList_;
        size_t poolSize_;
        size_t allocatedCount_;
    };

}
