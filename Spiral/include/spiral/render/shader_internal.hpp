#pragma once

#include "render_core.hpp"

namespace Spiral 
{
	struct Shader
	{
		uint32_t* source;
		size_t size;
		VkShaderStageFlagBits type;

		spirv_cross::ShaderResources resources;
	};
}