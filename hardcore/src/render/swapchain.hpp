#pragma once

#include "render_core.hpp"

#include <queue>

namespace ENGINE_NAMESPACE
{
	class swapchain
	{
	public:
		void init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
			u32 graphics_queue_idx, u32 present_queue_idx);
		void terminate(VkDevice device);
		void recreate(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
			u8 current_frame);
		void check_destroy_old(VkDevice device, u8 current_frame);

		~swapchain();

		inline const VkSwapchainKHR& vk_handle() const noexcept { return handle; }
		inline u32 size() const noexcept { return n_images; }
		inline VkFormat image_format() const noexcept { return surface_format.format; }
		inline const VkImageView* views() const noexcept { return image_views; }

		inline const VkExtent2D& extent() const noexcept { return _extent; }
		inline const VkViewport& viewport() const noexcept { return _viewport; }
		inline const VkRect2D& scissor() const noexcept { return _scissor; }

	private:
		VkSwapchainKHR handle;

		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR present_mode;

		VkExtent2D _extent;
		VkViewport _viewport;
		VkRect2D _scissor;

		std::array<u32, 2> queue_indexes;
		static const u32 render_copy = 0;
		static const u32 present = 1;

		VkImage* images = nullptr;
		u32 n_images;
		VkImageView* image_views = nullptr;

		struct old_swapchain
		{
			VkSwapchainKHR handle;
			u32 n_images;
			VkImageView* image_views;
			u8 deletion_frame;
		};

		std::queue<old_swapchain> old_swapchains;

		void update_dimensions(VkSurfaceCapabilitiesKHR& capabilities);
	};
}
