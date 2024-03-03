#pragma once

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE {
    class ENGINE_API memory_ref {
    public:
        memory_ref() = default;

        memory_ref(memory_ref &&ref) noexcept:
                m_pool_type(std::exchange(ref.m_pool_type, 0)),
                m_pool(std::exchange(ref.m_pool, std::numeric_limits<u32>::max())),
                m_offset(std::exchange(ref.m_offset, std::numeric_limits<std::size_t>::max())),
                m_size(std::exchange(ref.m_size, 0)) {}

        inline memory_ref &operator=(memory_ref &&ref) noexcept {
            m_pool_type = std::exchange(ref.m_pool_type, 0);
            m_pool = std::exchange(ref.m_pool, std::numeric_limits<u32>::max());
            m_offset = std::exchange(ref.m_offset, std::numeric_limits<std::size_t>::max());
            m_size = std::exchange(ref.m_size, 0);
            return *this;
        }

        memory_ref(const memory_ref &) = delete;

        memory_ref &operator=(const memory_ref &) = delete;

        inline bool operator==(memory_ref &other) const noexcept {
            return m_pool_type == other.m_pool_type && m_pool == other.m_pool &&
                   m_offset == other.m_offset && m_size == other.m_size;
        }

        [[nodiscard]] inline bool valid() const noexcept {
            return m_offset != std::numeric_limits<std::size_t>::max() && m_size != 0;
        }

        inline void invalidate() noexcept;

        [[nodiscard]] std::size_t hash() const noexcept;

    protected:
        memory_ref(u8 pool_type, u32 pool, std::size_t offset, std::size_t size) noexcept:
                m_pool_type(pool_type), m_pool(pool), m_offset(offset), m_size(size) {}

        u8 m_pool_type = 0;
        u32 m_pool = std::numeric_limits<u32>::max();
        std::size_t m_offset = std::numeric_limits<std::size_t>::max(); // Absolute offset in pool
        std::size_t m_size = 0;
    };
}
