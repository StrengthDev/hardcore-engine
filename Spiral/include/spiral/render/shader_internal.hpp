#pragma once

#include "render_core.hpp"
#include "shader.hpp"

namespace Spiral 
{
	class internal_shader : shader
	{
	public:
		internal_shader() = delete;

		internal_shader(const shader& other) : shader(other) {}

		//inline const std::uint32_t* get_data() const noexcept { return data; }
		//inline const std::size_t get_size() const noexcept { return size; }
		//inline const char* get_entry_point() const noexcept { return entry_point; }

		inline const VkShaderStageFlagBits get_stage() const
		{
			switch (stage)
			{
			case shader_t::VERTEX:		return VK_SHADER_STAGE_VERTEX_BIT;
			case shader_t::FRAGMENT:	return VK_SHADER_STAGE_FRAGMENT_BIT;
			default:
				//TODO: stuff
				DEBUG_BREAK;
			}
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}

		static VkShaderModule create_shader_module(const internal_shader& shader, VkDevice& handle, 
			VkShaderStageFlagBits* out_stage, const char** out_entry_point);
	};
}
