#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "shader_internal.hpp"
#include "memory.hpp"
#include "resource.hpp"

namespace Spiral
{
	class device;

	class graphics_pipeline
	{
	public:
		graphics_pipeline(device* owner, VkDevice logicalHandle, swapchain& swapchain, VkRenderPass& renderPass, 
			VkCommandPool commandPool, const Shader* shaders, uint16_t nShaders,
			const data_layout vertex_layout, const data_layout object_layout, const data_layout global_layout);
		~graphics_pipeline();

		void record_commands(VkCommandBuffer& buffer);

		device* owner = nullptr;

		VkDevice logicalHandle;
		swapchain& target;

		VkDescriptorSetLayout descriptorSetLayout;
		VkRenderPass render_pass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkExtent2D extent;
		VkFramebuffer* framebuffers;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet* descriptorSets;

		const data_layout vertex_layout;
		const data_layout object_layout;
		const data_layout global_layout;

		typedef object_resource::index_format index_format;

		struct object
		{
			object_resource* handle;
			std::uint32_t vertex_buffer;
			VkDeviceSize v_offset;
			VkDeviceSize size;
			std::uint32_t index_buffer;
			VkDeviceSize i_offset;
			index_format index_type;
			std::uint32_t count;
			std::uint32_t n_instances;
		};

		object* targets = nullptr;
		std::uint32_t n_targets = 0;

		bool valid;
	};
}
