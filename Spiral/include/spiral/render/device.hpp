#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"

namespace Spiral
{
	class device //TODO: SLI/crossfire support
	{
	public:
		device(VkPhysicalDevice physical, VkSurfaceKHR surface);
		device() = default;
		~device();

		bool create_swapchain();
		bool recreate_swapchain();

		inline device_memory& get_memory() { return memory; }

		bool draw();

	private:
		VkSurfaceKHR surface;
		VkPhysicalDevice physical_handle;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkDevice handle;
		bool has_handle = false;
		uint32_t score = 0; //TODO: calculate score

		typedef uint32_t queue_idx_t;
		const queue_idx_t invalid_queue_idx = std::numeric_limits<queue_idx_t>::max();

		//Queue families
		queue_idx_t graphicsIndex = invalid_queue_idx;
		VkQueue graphicsQueue = VK_NULL_HANDLE; //dedicated to graphics operations, graphics related compute operations and device->device transfers

		queue_idx_t presentIndex = invalid_queue_idx;
		VkQueue presentQueue = VK_NULL_HANDLE; //ideally the same as graphics queue

		queue_idx_t computeIndex = invalid_queue_idx;
		VkQueue computeQueue = VK_NULL_HANDLE; //dedicated to compute operations unrelated to graphics

		queue_idx_t transfer_idx = invalid_queue_idx;
		VkQueue transfer_queue = VK_NULL_HANDLE; //dedicated to host->device transfers and vice versa

		device_memory memory;

		typedef uint16_t index_t;

		//Frame data
		index_t current_frame = 0;
		VkSemaphore image_available_semaphores[max_frames_in_flight] = {};
		VkSemaphore render_finished_semaphores[max_frames_in_flight] = {};
		VkFence frame_fences[max_frames_in_flight] = {};

		bool has_swapchain = false;
		swapchain main_swapchain;
		VkRenderPass render_pass = VK_NULL_HANDLE;
		VkFramebuffer* framebuffers = nullptr;

		VkCommandPool* graphics_command_pools = nullptr;
		index_t n_graphics_command_pools;

		VkCommandBuffer* graphics_command_buffers = nullptr;

		index_t n_graphics_pipelines = 0;
		graphics_pipeline* graphics_pipelines = nullptr;

		void record_secondary_graphics(index_t buffer_idx, uint32_t image_index);

		friend device_memory;
		friend swapchain;
	};
}
