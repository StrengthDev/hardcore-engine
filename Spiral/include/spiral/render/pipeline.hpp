#pragma once

#include <spiral/core/core.hpp>

namespace Spiral
{
	class SPIRAL_API pipeline
	{
	public:
		pipeline() = delete;
		pipeline(const pipeline&) = delete;

		pipeline(const char* vert, const char* frag); //TEMP

		~pipeline();

	private:
		bool valid = false;
	};
}
