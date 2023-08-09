#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"

namespace ENGINE_NAMESPACE
{
	class device
	{

		
	public:
		device(VkPhysicalDevice&& physical, VkSurfaceKHR& surface);
		device() = default;
		~device();

		std::size_t score();

		void create_swapchain();
		void recreate_swapchain();

		bool draw();

		inline device_memory& get_memory() noexcept { return memory; }

		u32 add_graphics_pipeline(const shader& vertex, const shader& fragment);
		void add_instanced_graphics_pipeline();
		void add_indirect_graphics_pipeline();

		inline graphics_pipeline& get_graphics_pipeline(u32 idx) { return graphics_pipelines[idx]; }

		device(const device&) = delete;
		device& operator=(const device&) = delete;

		device(device&& other) noexcept :
			surface(std::exchange(other.surface, nullptr)), physical_handle(std::exchange(other.physical_handle, VK_NULL_HANDLE)),
			properties(std::exchange(other.properties, {})), features(std::exchange(other.features, {})),
			handle(std::exchange(other.handle, VK_NULL_HANDLE)),
			graphics_idx(std::exchange(other.graphics_idx, 0)), graphics_queue(std::exchange(other.graphics_queue, VK_NULL_HANDLE)),
			present_idx(std::exchange(other.present_idx, 0)), present_queue(std::exchange(other.present_queue, VK_NULL_HANDLE)),
			compute_idx(std::exchange(other.compute_idx, 0)), compute_queue(std::exchange(other.compute_queue, VK_NULL_HANDLE)),
			transfer_idx(std::exchange(other.transfer_idx, 0)), transfer_queue(std::exchange(other.transfer_queue, VK_NULL_HANDLE)),
			memory(std::move(other.memory)), current_frame(std::exchange(other.current_frame, 0)), 
			image_available_semaphores(std::move(other.image_available_semaphores)),
			render_finished_semaphores(std::move(other.render_finished_semaphores)),
			frame_fences(std::move(other.frame_fences)),
			has_swapchain(std::exchange(other.has_swapchain, false)),
			main_swapchain(std::move(other.main_swapchain)),
			render_pass(std::exchange(other.render_pass, VK_NULL_HANDLE)),
			framebuffers(std::exchange(other.framebuffers, nullptr)),
			graphics_command_pools(std::exchange(other.graphics_command_pools, nullptr)),
			command_parallelism(std::exchange(other.command_parallelism, 0)),
			pipeline_stacks(std::move(other.pipeline_stacks)),
			graphics_command_buffers(std::exchange(other.graphics_command_buffers, nullptr)),
			graphics_pipelines(std::move(other.graphics_pipelines))
		{
			reset_ownerships();
		}

	private:
		VkSurfaceKHR* surface = nullptr;
		VkPhysicalDevice physical_handle = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkDevice handle = VK_NULL_HANDLE;

		typedef u32 queue_idx_t;
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
		u8 current_frame = 0;
		std::array<VkSemaphore, max_frames_in_flight> image_available_semaphores;
		std::array<VkSemaphore, max_frames_in_flight> render_finished_semaphores;
		std::array<VkFence, max_frames_in_flight> frame_fences;

		bool has_swapchain = false; //TODO remove
		swapchain main_swapchain;
		VkRenderPass render_pass = VK_NULL_HANDLE;
		VkFramebuffer* framebuffers = nullptr;

		struct old_framebuffer_set
		{
			VkFramebuffer* framebuffers;
			u32 n_framebuffers;
			u8 deletion_frame;
		};

		std::queue<old_framebuffer_set> old_framebuffers;

		VkCommandPool* graphics_command_pools = nullptr;
		u32 command_parallelism;
		std::vector<std::vector<graphics_pipeline*>> pipeline_stacks;

		VkCommandBuffer* graphics_command_buffers = nullptr;

		std::vector<graphics_pipeline> graphics_pipelines;

		void record_secondary_graphics(VkCommandBuffer& buffer, u32 image_index,
			std::vector<graphics_pipeline*>& graphics_pipelines);

		inline void reset_ownerships()
		{
			memory.update_refs(handle, &properties.limits, &current_frame);
			//TODO add other objects using owner
		}

		//TODO: remove friends, use inline acessors
		friend graphics_pipeline;
	};
}
