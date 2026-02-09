// src/domain/memory/PoolAllocator.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

namespace winsetup::domain {

    template<typename T, size_t BlockSize = 4096>
    class PoolAllocator {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        PoolAllocator() noexcept = default;

        template<typename U>
        PoolAllocator(const PoolAllocator<U, BlockSize>&) noexcept {}

        [[nodiscard]] pointer allocate(size_type n) {
            if (n == 0) {
                return nullptr;
            }

            if (n > BlockSize) {
                return static_cast<pointer>(::operator new(n * sizeof(T)));
            }

            std::lock_guard lock(m_mutex);

            if (m_freeList.empty()) {
                AllocateBlock();
            }

            pointer result = m_freeList.back();
            m_freeList.pop_back();
            return result;
        }

        void deallocate(pointer p, size_type n) noexcept {
            if (!p) {
                return;
            }

            if (n > BlockSize) {
                ::operator delete(p);
                return;
            }

            std::lock_guard lock(m_mutex);
            m_freeList.push_back(p);
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            new (p) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) {
            p->~U();
        }

    private:
        void AllocateBlock() {
            auto block = std::make_unique<std::byte[]>(BlockSize * sizeof(T));

            for (size_t i = 0; i < BlockSize; ++i) {
                m_freeList.push_back(
                    reinterpret_cast<pointer>(block.get() + i * sizeof(T))
                );
            }

            m_blocks.push_back(std::move(block));
        }

        std::vector<pointer> m_freeList;
        std::vector<std::unique_ptr<std::byte[]>> m_blocks;
        std::mutex m_mutex;
    };

}
