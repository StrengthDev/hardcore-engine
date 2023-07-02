#pragma once

#include "render_core.hpp"
#include <render/shader.hpp>

namespace ENGINE_NAMESPACE
{
	class internal_shader : public shader
	{
	public:
		internal_shader() = delete;

		inline VkShaderStageFlagBits get_stage() const noexcept
		{
			switch (stage)
			{
			case shader_t::VERTEX:					return VK_SHADER_STAGE_VERTEX_BIT;
			case shader_t::FRAGMENT:				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case shader_t::COMPUTE:					return VK_SHADER_STAGE_COMPUTE_BIT;
			case shader_t::TESSELATION_CONTROL:		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case shader_t::TESSELATION_EVALUATION:	return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case shader_t::GEOMETRY:				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case shader_t::RAY_GENERATION:			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			case shader_t::RAY_INTERSECTION:		return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			case shader_t::RAY_ANY_HIT:				return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			case shader_t::RAY_CLOSEST_HIT:			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			case shader_t::RAY_MISS:				return VK_SHADER_STAGE_MISS_BIT_KHR;
			case shader_t::RAY_CALLABLE:			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
			case shader_t::MESH:					return VK_SHADER_STAGE_MESH_BIT_NV;
			case shader_t::TASK:					return VK_SHADER_STAGE_TASK_BIT_NV;
			default:
				INTERNAL_ASSERT(false, "Invalid shader stage");
			}
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}

		inline const std::vector<data_layout>& inputs() const noexcept { return _inputs; }

		inline const std::vector<descriptor_data>& descriptors() const noexcept { return _descriptors; }

		static VkShaderModule create_shader_module(const shader& shader, VkDevice& handle, 
			VkShaderStageFlagBits* out_stage, const char** out_entry_point);
	};
}
