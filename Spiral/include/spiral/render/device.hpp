#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"

namespace ENGINE_NAMESPACE
{
	class device
	{

		typedef std::uint16_t index_t;
	public:
		device(VkPhysicalDevice&& physical, VkSurfaceKHR& surface);
		device() = default;
		~device();

		std::size_t score();

		bool create_swapchain();
		bool recreate_swapchain();

		bool draw();

		inline device_memory& get_memory() noexcept { return memory; }

		index_t add_graphics_pipeline(const shader& vertex, const shader& fragment);
		void add_instanced_graphics_pipeline();
		void add_indirect_graphics_pipeline();

		inline graphics_pipeline& get_graphics_pipeline(std::uint32_t idx) { return graphics_pipelines[idx]; }

		device(const device&) = delete;
		device& operator=(const device&) = delete;

	private:
		VkSurfaceKHR* surface;
		VkPhysicalDevice physical_handle;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkDevice handle = VK_NULL_HANDLE;

		typedef std::uint32_t queue_idx_t;
		static constexpr queue_idx_t invalid_queue_idx = std::numeric_limits<queue_idx_t>::max();

		//Queue families

		//dedicated to graphics operations, graphics related compute operations and device->device transfers
		queue_idx_t graphics_idx = invalid_queue_idx;
		VkQueue graphics_queue = VK_NULL_HANDLE;

		//ideally the same as graphics queue
		queue_idx_t present_idx = invalid_queue_idx;
		VkQueue present_queue = VK_NULL_HANDLE;

		//dedicated to compute operations unrelated to graphics
		queue_idx_t compute_idx = invalid_queue_idx;
		VkQueue compute_queue = VK_NULL_HANDLE;

		//dedicated to host->device transfers and vice versa
		queue_idx_t transfer_idx = invalid_queue_idx;
		VkQueue transfer_queue = VK_NULL_HANDLE;

		device_memory memory;

		//Frame data
		std::uint8_t current_frame = 0;
		VkSemaphore image_available_semaphores[max_frames_in_flight] = {};
		VkSemaphore render_finished_semaphores[max_frames_in_flight] = {};
		VkFence frame_fences[max_frames_in_flight] = {};

		bool has_swapchain = false; //TODO remove
		swapchain main_swapchain;
		VkRenderPass render_pass = VK_NULL_HANDLE;
		VkFramebuffer* framebuffers = nullptr;

		VkCommandPool* graphics_command_pools = nullptr;
		std::uint32_t command_parallelism;

		VkCommandBuffer* graphics_command_buffers = nullptr;

		std::vector<graphics_pipeline> graphics_pipelines;

		void record_secondary_graphics(VkCommandBuffer& buffer, std::uint32_t image_index,
			const std::vector<graphics_pipeline*>& graphics_pipelines);

		//TODO: remove friends, use inline acessors
		friend device_memory;
		friend swapchain;
		friend graphics_pipeline;
	};
}
