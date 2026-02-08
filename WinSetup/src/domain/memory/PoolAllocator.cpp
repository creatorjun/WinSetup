// src/domain/memory/PoolAllocator.cpp

#include <domain/memory/PoolAllocator.h>
#include <algorithm>

namespace winsetup::domain {

    PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
        : m_blockSize(std::max(blockSize, sizeof(Block)))
        , m_totalBlocks(blockCount)
        , m_freeList(nullptr)
    {
        AllocatePool();
    }

    PoolAllocator::~PoolAllocator() {
    }

    void PoolAllocator::AllocatePool() {
        auto pool = std::make_unique<std::byte[]>(m_blockSize * m_totalBlocks);
        std::byte* poolPtr = pool.get();

        for (size_t i = 0; i < m_totalBlocks; ++i) {
            Block* block = reinterpret_cast<Block*>(poolPtr + i * m_blockSize);
            block->next = m_freeList;
            m_freeList = block;
        }

        m_pools.push_back(std::move(pool));
    }

    void* PoolAllocator::Allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_freeList) {
            AllocatePool();
        }

        Block* block = m_freeList;
        m_freeList = m_freeList->next;

        return block;
    }

    void PoolAllocator::Deallocate(void* ptr) noexcept {
        if (!ptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        Block* block = static_cast<Block*>(ptr);
        block->next = m_freeList;
        m_freeList = block;
    }

    size_t PoolAllocator::GetUsedBlocks() const noexcept {
        return m_totalBlocks * m_pools.size() - GetFreeBlocks();
    }

    size_t PoolAllocator::GetFreeBlocks() const noexcept {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));

        size_t count = 0;
        Block* current = m_freeList;
        while (current) {
            ++count;
            current = current->next;
        }

        return count;
    }

}
