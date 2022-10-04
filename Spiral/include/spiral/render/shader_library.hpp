#pragma once

#include "shader.hpp"

namespace ENGINE_NAMESPACE
{
	namespace shader_library
	{
		ENGINE_API const shader& add(shader&& s);
		ENGINE_API const shader& get(const char* name);
		ENGINE_API bool has(const char* name);
	};
}
