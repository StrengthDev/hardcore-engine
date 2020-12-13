#pragma once

#include <cstdint>

//TODO: make this a class dummy
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