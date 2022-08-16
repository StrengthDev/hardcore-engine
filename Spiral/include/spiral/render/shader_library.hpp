#pragma once

#include "shader.hpp"

namespace Spiral
{
	namespace shader_library
	{
		//void init();
		//void terminate();

		SPIRAL_API const shader& add(shader&& s);
		SPIRAL_API const shader& get(const char* name);
		SPIRAL_API bool has(const char* name);
	};
}
