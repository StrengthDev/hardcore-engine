#pragma once

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE
{
	class ENGINE_API memory_ref
	{
	public:
		memory_ref() = default;

		memory_ref(memory_ref&& ref) noexcept : 
			pool_type(std::exchange(ref.pool_type, 0)),
			pool(std::exchange(ref.pool, std::numeric_limits<u32>::max())), 
			offset(std::exchange(ref.offset, std::numeric_limits<std::size_t>::max())), 
			size(std::exchange(ref.size, 0)) 
		{}

		inline memory_ref& operator=(memory_ref&& ref) noexcept
		{
			pool_type = std::exchange(ref.pool_type, 0);
			pool = std::exchange(ref.pool, std::numeric_limits<u32>::max());
			offset = std::exchange(ref.offset, std::numeric_limits<std::size_t>::max());
			size = std::exchange(ref.size, 0);
			return *this;
		}

		memory_ref(const memory_ref&) = delete;
		memory_ref& operator=(const memory_ref&) = delete;

		inline bool operator==(memory_ref& ref) const noexcept
		{
			return pool_type == ref.pool_type && pool == ref.pool && offset == ref.offset && size == ref.size;
		}

		inline bool valid() const noexcept { return offset != std::numeric_limits<std::size_t>::max() && size != 0; }

		inline void invalidate() noexcept
		{
			pool_type = 0;
			pool = std::numeric_limits<u32>::max();
			offset = std::numeric_limits<std::size_t>::max();
			size = 0;
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
			return (offset << (size_bitwidth + pool_bitwidth + pool_type_bitwidth)) ^
				(size << (pool_bitwidth + pool_type_bitwidth)) ^
				(static_cast<std::size_t>(pool) << pool_type_bitwidth) ^
				pool_type;
		}

	protected:
		memory_ref(u8 pool_type, u32 pool, std::size_t offset, std::size_t size) noexcept :
			pool_type(pool_type), pool(pool), offset(offset), size(size) 
		{ }

		u8 pool_type = 0;
		u32 pool = std::numeric_limits<u32>::max();
		std::size_t offset = std::numeric_limits<std::size_t>::max(); //absolute offset in pool
		std::size_t size = 0;
	};
}
