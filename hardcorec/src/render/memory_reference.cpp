#include <pch.hpp>

#include "memory.hpp"

#include <hardcore/render/memory_reference.hpp>

namespace ENGINE_NAMESPACE {
    void memory_ref::invalidate() noexcept {
        m_pool_type = 0;
        m_pool = std::numeric_limits<u32>::max();
        m_offset = std::numeric_limits<std::size_t>::max();
        m_size = 0;
    }

    constexpr u64 log2(u64 val) {
        u64 exp = 2;
        u64 log = 1;

        while (exp < val) {
            exp = exp << 1;
            log++;
        }

        return log;
    }

    std::size_t memory_ref::hash() const noexcept {
        constexpr u64 pool_type_bitwidth = log2(device_memory::buffer_t::ENUM_COUNT);
        constexpr u64 pool_bitwidth = 12;
        constexpr u64 offset_bitwidth = 24;
        //constexpr u64 size_bitwidth = sizeof(std::size_t) * 8 - pool_type_bitwidth - pool_bitwidth - size_bitwidth;

        //constexpr u64 pool_type_max = BIT(pool_type_bitwidth) - 1;
        //constexpr u64 pool_max = BIT(pool_bitwidth) - 1;
        //constexpr u64 offset_max = BIT(offset_bitwidth) - 1;
        //constexpr u64 size_max = BIT(size_bitwidth) - 1;

        //	    size(16777215)	  -		offset(16777215)   -	pool(4095)	  - type(15)
        // 0000000000000000000000 - 0000000000000000000000 - 0000000000000000 - 0000
        return (m_size << (offset_bitwidth + pool_bitwidth + pool_type_bitwidth)) ^
               (m_offset << (pool_bitwidth + pool_type_bitwidth)) ^
               (static_cast<std::size_t>(m_pool) << pool_type_bitwidth) ^
               m_pool_type;
    }
}
