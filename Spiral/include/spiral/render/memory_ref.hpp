#pragma once

#include <spiral/core/core.hpp>

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

		memory_ref& operator=(memory_ref&& ref) noexcept
		{
			pool_type = std::exchange(ref.pool_type, 0);
			pool = std::exchange(ref.pool, std::numeric_limits<std::uint32_t>::max());
			offset = std::exchange(ref.offset, std::numeric_limits<std::size_t>::max());
			size = std::exchange(ref.size, 0);
			return *this;
		}

		memory_ref(const memory_ref&) = delete;
		memory_ref& operator=(const memory_ref&) = delete;

		inline bool valid() const noexcept { return offset != std::numeric_limits<std::size_t>::max() && size != 0; }

		inline void invalidate() noexcept
		{
			pool_type = 0;
			pool = std::numeric_limits<std::uint32_t>::max();
			offset = std::numeric_limits<std::size_t>::max();
			size = 0;
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
