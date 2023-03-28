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
			pool(std::exchange(ref.pool, std::numeric_limits<std::uint32_t>::max())), 
			offset(std::exchange(ref.offset, std::numeric_limits<std::size_t>::max())), 
			size(std::exchange(ref.size, 0)) 
		{}

		inline memory_ref& operator=(memory_ref&& ref) noexcept
		{
			pool_type = std::exchange(ref.pool_type, 0);
			pool = std::exchange(ref.pool, std::numeric_limits<std::uint32_t>::max());
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
			pool = std::numeric_limits<std::uint32_t>::max();
			offset = std::numeric_limits<std::size_t>::max();
			size = 0;
		}

		inline std::size_t hash() const noexcept 
		{
			return (offset << 42) ^ (size << 20) ^ (static_cast<std::size_t>(pool) << 4) ^ pool_type;
		}

	protected:
		memory_ref(std::uint8_t pool_type, std::uint32_t pool, std::size_t offset, std::size_t size) noexcept :
			pool_type(pool_type), pool(pool), offset(offset), size(size) {}

		std::uint8_t pool_type = 0;
		std::uint32_t pool = std::numeric_limits<std::uint32_t>::max();
		std::size_t offset = std::numeric_limits<std::size_t>::max();
		std::size_t size = 0;
	};
}
