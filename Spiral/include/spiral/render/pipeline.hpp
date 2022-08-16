#pragma once

#include <spiral/core/core.hpp>

namespace Spiral
{
	enum class pipeline_t : std::uint8_t
	{
		RENDER = 0,
		PIXEL,
		COMPUTE
	};

	enum class pipeline_s : std::uint8_t
	{
		DISABLED = 0,
		ACTIVE,
		PASSIVE
	};

	class SPIRAL_API pipeline
	{
	public:
		pipeline() = delete;
		pipeline(const pipeline&) = delete;

		pipeline(const char* vert, const char* frag); //TEMP

		~pipeline();

		inline bool valid() const
		{
			return id != std::numeric_limits<std::uint32_t>::max();
		}

	private:
		std::uint32_t id = std::numeric_limits<std::uint32_t>::max();
		pipeline_s status = pipeline_s::DISABLED;

	public:
		const pipeline_t type;
	};
}
