#pragma once

#include <cstdint>

namespace Spiral
{
	struct Mesh
	{
		void* vertices;
		uint32_t vSize;
		void* indices;
		uint32_t iSize;
	};
}