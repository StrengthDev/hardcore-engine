#pragma once

#include "Spiral/Core/Core.hpp"
#include <cstdint>

namespace Spiral
{
	enum class ShaderType : uint16_t
	{
		None = 0,
		Vertex, Fragment
	};

	namespace ShaderLibrary
	{
		uint32_t SPIRAL_API load(const char* filename, ShaderType type);
	}
}