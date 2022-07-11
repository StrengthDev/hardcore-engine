#pragma once

namespace Spiral
{
	class memory_reference
	{
	public:
		memory_reference() = default;

		memory_reference(memory_reference&& ref) noexcept : 
			pool(std::exchange(ref.pool, std::numeric_limits<std::uint32_t>::max())), 
			offset(std::exchange(ref.offset, std::numeric_limits<std::size_t>::max())), 
			size(std::exchange(ref.size, 0)) 
		{}

		memory_reference& operator=(memory_reference&& ref) noexcept
		{
			pool = std::exchange(ref.pool, std::numeric_limits<std::uint32_t>::max());
			offset = std::exchange(ref.offset, std::numeric_limits<std::size_t>::max());
			size = std::exchange(ref.size, 0);
			return *this;
		}

		memory_reference(const memory_reference&) = delete;
		memory_reference& operator=(const memory_reference&) = delete;

		inline bool valid() const noexcept { return offset != std::numeric_limits<std::size_t>::max() && size != 0; }

		inline void invalidate() noexcept
		{
			pool = std::numeric_limits<std::uint32_t>::max();
			offset = std::numeric_limits<std::size_t>::max();
			size = 0;
		}

	protected:
		memory_reference(std::uint32_t pool, std::size_t offset, std::size_t size) noexcept : pool(pool), offset(offset), size(size) {}

		std::uint32_t pool = std::numeric_limits<std::uint32_t>::max();
		std::size_t offset = std::numeric_limits<std::size_t>::max();
		std::size_t size = 0;
	};
}
