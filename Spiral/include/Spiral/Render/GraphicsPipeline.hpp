#pragma once

#include "RenderCore.hpp"
#include "swapchain.hpp"
#include "shader.hpp"
#include "memory.hpp"
#include "mesh.hpp"

namespace Spiral
{
	struct GraphicsPipeline
	{
		VkDescriptorSetLayout descriptorSetLayout;
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		//TODO: each pipeline must have its own command pool
		VkCommandBuffer* commandBuffers;
		uint32_t nCommandBuffers;

		VkExtent2D extent;
		VkFramebuffer* frameBuffers;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet* descriptorSets;

		MemoryNexus memory;

		bool valid;

		void init(VkDevice logicalHandle, Swapchain swapchain, VkCommandPool commandPool, const Shader* shaders, uint16_t nShaders);
		void terminate(VkDevice logicalHandle, uint32_t nImages);

		void loadMesh(Mesh mesh);

		void draw(size_t frame);
	};
}