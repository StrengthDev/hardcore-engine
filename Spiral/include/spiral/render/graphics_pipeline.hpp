#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "shader_internal.hpp"
#include "memory.hpp"
#include "resource.hpp"
#include "instance.hpp"

namespace ENGINE_NAMESPACE
{
	class device;

	class graphics_pipeline
	{
	public:
		graphics_pipeline(device& owner, const std::vector<const shader*>& shaders);
		~graphics_pipeline();

		graphics_pipeline(const graphics_pipeline&) = delete;
		inline graphics_pipeline& operator=(const graphics_pipeline&) = delete;

		graphics_pipeline(graphics_pipeline&& other) noexcept : owner(other.owner), 
			descriptor_set_layouts(std::exchange(other.descriptor_set_layouts, VK_NULL_HANDLE)),
			pipeline_layout(std::exchange(other.pipeline_layout, VK_NULL_HANDLE)),
			handle(std::exchange(other.handle, VK_NULL_HANDLE)),
			n_descriptor_pool_sizes{ std::exchange(other.n_descriptor_pool_sizes[0], 0), 
				std::exchange(other.n_descriptor_pool_sizes[1], 0) },
			descriptor_pool_sizes{ std::exchange(other.descriptor_pool_sizes[0], nullptr),
				std::exchange(other.descriptor_pool_sizes[1], nullptr) },
			descriptor_pool(std::exchange(other.descriptor_pool, VK_NULL_HANDLE)),
			descriptor_sets(std::exchange(other.descriptor_sets, nullptr)),
			objects(std::exchange(other.objects, std::vector<object>(0)))
		{ }

		inline graphics_pipeline& operator=(graphics_pipeline&& other) noexcept
		{
			this->~graphics_pipeline();
			owner = other.owner;
			descriptor_set_layouts = std::exchange(other.descriptor_set_layouts, VK_NULL_HANDLE);
			pipeline_layout = std::exchange(other.pipeline_layout, VK_NULL_HANDLE);
			handle = std::exchange(other.handle, VK_NULL_HANDLE);
			n_descriptor_pool_sizes[0] = std::exchange(other.n_descriptor_pool_sizes[0], 0);
			n_descriptor_pool_sizes[1] = std::exchange(other.n_descriptor_pool_sizes[1], 0);
			descriptor_pool_sizes[0] = std::exchange(other.descriptor_pool_sizes[0], nullptr);
			descriptor_pool_sizes[1] = std::exchange(other.descriptor_pool_sizes[1], nullptr);
			descriptor_pool = std::exchange(other.descriptor_pool, VK_NULL_HANDLE);
			descriptor_sets = std::exchange(other.descriptor_sets, nullptr);
			objects = std::exchange(other.objects, std::vector<object>(0));
			return *this;
		}

		void record_commands(VkCommandBuffer& buffer, std::uint8_t current_frame);

		void link(const object_resource& object);
		void link(const object_resource& object, const device_data& instances);

	private:
		device* owner;

		std::uint32_t n_descriptor_set_layouts = 0;
		VkDescriptorSetLayout* descriptor_set_layouts = nullptr;
		VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
		VkPipeline handle = VK_NULL_HANDLE;

		std::uint32_t n_descriptor_pool_sizes[2] = { 0, 0 };
		VkDescriptorPoolSize* descriptor_pool_sizes[2] = { nullptr, nullptr };

		VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
		VkDescriptorSet* descriptor_sets = nullptr;
		std::uint32_t object_descriptor_set_capacity = 0;

		struct object
		{
			const object_resource* mesh = nullptr;
			std::uint32_t instances = 1;
		};

		std::vector<object> objects;
	};
}
