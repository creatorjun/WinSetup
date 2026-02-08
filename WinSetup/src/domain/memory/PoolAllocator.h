// src/domain/memory/PoolAllocator.h
#pragma once

#include <cstddef>
#include <vector>
#include <mutex>
#include <memory>

namespace winsetup::domain {

    template<typename T, size_t BlockSize = 4096>
    class PoolAllocator {
    public:
        PoolAllocator() {
            m_blocks.reserve(4);
            AllocateBlock();
        }

        ~PoolAllocator() {
            for (auto* block : m_blocks) {
                ::operator delete(block);
            }
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        T* Allocate() {
            std::lock_guard lock(m_mutex);

            if (m_freeList.empty()) {
                AllocateBlock();
            }

            T* ptr = m_freeList.back();
            m_freeList.pop_back();
            return ptr;
        }

        void Deallocate(T* ptr) noexcept {
            if (!ptr) return;

            std::lock_guard lock(m_mutex);
            m_freeList.push_back(ptr);
        }

        template<typename... Args>
        T* Construct(Args&&... args) {
            T* ptr = Allocate();
            try {
                new (ptr) T(std::forward<Args>(args)...);
                return ptr;
            }
            catch (...) {
                Deallocate(ptr);
                throw;
            }
        }

        void Destroy(T* ptr) noexcept {
            if (!ptr) return;
            ptr->~T();
            Deallocate(ptr);
        }

        [[nodiscard]] size_t GetBlockCount() const noexcept {
            return m_blocks.size();
        }

        [[nodiscard]] size_t GetFreeCount() const noexcept {
            std::lock_guard lock(m_mutex);
            return m_freeList.size();
        }

    private:
        void AllocateBlock() {
            constexpr size_t elementSize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
            constexpr size_t elementsPerBlock = BlockSize / elementSize;

            void* block = ::operator new(BlockSize);
            m_blocks.push_back(block);

            char* ptr = static_cast<char*>(block);
            for (size_t i = 0; i < elementsPerBlock; ++i) {
                m_freeList.push_back(reinterpret_cast<T*>(ptr + i * elementSize));
            }
        }

        std::vector<void*> m_blocks;
        std::vector<T*> m_freeList;
        mutable std::mutex m_mutex;
    };

}
