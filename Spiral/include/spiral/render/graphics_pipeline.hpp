#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "shader_internal.hpp"
#include "memory.hpp"
#include "resource.hpp"
#include "instance.hpp"

namespace Spiral
{
	class device;

	class graphics_pipeline
	{
	public:
		graphics_pipeline(device& owner, const shader** shaders, uint16_t nShaders,
			const data_layout& vertex_layout, const data_layout& object_layout, const data_layout& global_layout);
		~graphics_pipeline();

		graphics_pipeline(const graphics_pipeline&) = delete;
		inline graphics_pipeline& operator=(const graphics_pipeline&) = delete;

		graphics_pipeline(graphics_pipeline&& other) noexcept : owner(other.owner), 
			descriptorSetLayout(std::exchange(other.descriptorSetLayout, VK_NULL_HANDLE)),
			pipelineLayout(std::exchange(other.pipelineLayout, VK_NULL_HANDLE)),
			graphicsPipeline(std::exchange(other.graphicsPipeline, VK_NULL_HANDLE)),
			descriptorPool(std::exchange(other.descriptorPool, VK_NULL_HANDLE)),
			descriptorSets(std::exchange(other.descriptorSets, nullptr)),
			objects(std::exchange(other.objects, std::vector<const object_resource*>(0))), 
			instances(std::exchange(other.instances, std::vector<const instance*>(0))),
			use_instancing(std::exchange(other.use_instancing, false))
		{ }

		inline graphics_pipeline& operator=(graphics_pipeline&& other) noexcept
		{
			this->~graphics_pipeline();
			owner = other.owner;
			descriptorSetLayout = std::exchange(other.descriptorSetLayout, VK_NULL_HANDLE);
			pipelineLayout = std::exchange(other.pipelineLayout, VK_NULL_HANDLE);
			graphicsPipeline = std::exchange(other.graphicsPipeline, VK_NULL_HANDLE);
			descriptorPool = std::exchange(other.descriptorPool, VK_NULL_HANDLE);
			descriptorSets = std::exchange(other.descriptorSets, nullptr);
			objects = std::exchange(other.objects, std::vector<const object_resource*>(0));
			instances = std::exchange(other.instances, std::vector<const instance*>(0));
			use_instancing = std::exchange(other.use_instancing, false);
			return *this;
		}

		void record_commands(VkCommandBuffer& buffer);

		void link(const object_resource& object);
		void link(const object_resource& object, const instance& instance);

	private:
		device* owner;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;

		VkDescriptorPool descriptorPool;
		VkDescriptorSet* descriptorSets;

		std::vector<const object_resource*> objects;
		std::vector<const instance*> instances;
		bool use_instancing = false;
	};
}
