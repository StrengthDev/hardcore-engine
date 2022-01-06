#pragma once

#include "Shader.hpp"
#include "RenderCore.hpp"

#define INITIAL_NUM_SHADERS 10
#define INCREASE_STEP 10

namespace Spiral
{
	namespace ShaderLibrary
	{
		void init();
		void terminate();

		uint32_t add(Shader s);
		Shader& get(int i);
	};
}