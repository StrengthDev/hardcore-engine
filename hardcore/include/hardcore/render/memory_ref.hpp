#pragma once

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE
{
	class ENGINE_API memory_ref
	{
	public:
		memory_ref() = default;

		memory_ref(memory_ref&& ref) noexcept : 
			m_pool_type(std::exchange(ref.m_pool_type, 0)),
			m_pool(std::exchange(ref.m_pool, std::numeric_limits<u32>::max())), 
			m_offset(std::exchange(ref.m_offset, std::numeric_limits<std::size_t>::max())), 
			m_size(std::exchange(ref.m_size, 0)) 
		{}

		inline memory_ref& operator=(memory_ref&& ref) noexcept
		{
			m_pool_type = std::exchange(ref.m_pool_type, 0);
			m_pool = std::exchange(ref.m_pool, std::numeric_limits<u32>::max());
			m_offset = std::exchange(ref.m_offset, std::numeric_limits<std::size_t>::max());
			m_size = std::exchange(ref.m_size, 0);
			return *this;
		}

		memory_ref(const memory_ref&) = delete;
		memory_ref& operator=(const memory_ref&) = delete;

		inline bool operator==(memory_ref& other) const noexcept
		{
			return m_pool_type == other.m_pool_type && m_pool == other.m_pool && 
				m_offset == other.m_offset && m_size == other.m_size;
		}

		inline bool valid() const noexcept { return m_offset != std::numeric_limits<std::size_t>::max() && m_size != 0; }

		inline void invalidate() noexcept
		{
			m_pool_type = 0;
			m_pool = std::numeric_limits<u32>::max();
			m_offset = std::numeric_limits<std::size_t>::max();
			m_size = 0;
		}

		inline std::size_t hash() const noexcept 
		{
			constexpr u64 pool_type_bitwidth = 4;
			constexpr u64 pool_bitwidth = 12;
			constexpr u64 size_bitwidth = 24;
			//constexpr u64 offset_bitwidth = sizeof(std::size_t) * 8 - pool_type_bitwidth - pool_bitwidth - size_bitwidth;

			//constexpr u64 pool_type_max = BIT(pool_type_bitwidth) - 1;
			//constexpr u64 pool_max = BIT(pool_bitwidth) - 1;
			//constexpr u64 size_max = BIT(size_bitwidth) - 1;
			//constexpr u64 offset_max = BIT(offset_bitwidth) - 1;

			//	offset(16777215)	  -		size(16777215)	   -	pool(4095)	  - type(15)
			// 0000000000000000000000 - 0000000000000000000000 - 0000000000000000 - 0000
			return (m_offset << (size_bitwidth + pool_bitwidth + pool_type_bitwidth)) ^
				(m_size << (pool_bitwidth + pool_type_bitwidth)) ^
				(static_cast<std::size_t>(m_pool) << pool_type_bitwidth) ^
				m_pool_type;
		}

	protected:
		memory_ref(u8 pool_type, u32 pool, std::size_t offset, std::size_t size) noexcept :
			m_pool_type(pool_type), m_pool(pool), m_offset(offset), m_size(size) 
		{ }

		u8 m_pool_type = 0;
		u32 m_pool = std::numeric_limits<u32>::max();
		std::size_t m_offset = std::numeric_limits<std::size_t>::max(); // absolute offset in pool
		std::size_t m_size = 0;
	};
}
