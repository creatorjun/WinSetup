// src/domain/memory/PoolAllocator.h

#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>

namespace winsetup::domain {

    class PoolAllocator {
    public:
        explicit PoolAllocator(size_t blockSize, size_t blockCount = 128);
        ~PoolAllocator();

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        void* Allocate();
        void Deallocate(void* ptr) noexcept;

        [[nodiscard]] size_t GetBlockSize() const noexcept { return m_blockSize; }
        [[nodiscard]] size_t GetTotalBlocks() const noexcept { return m_totalBlocks; }
        [[nodiscard]] size_t GetUsedBlocks() const noexcept;
        [[nodiscard]] size_t GetFreeBlocks() const noexcept;

    private:
        struct Block {
            Block* next = nullptr;
        };

        void AllocatePool();

        size_t m_blockSize;
        size_t m_totalBlocks;
        std::vector<std::unique_ptr<std::byte[]>> m_pools;
        Block* m_freeList;
        std::mutex m_mutex;
    };

}
