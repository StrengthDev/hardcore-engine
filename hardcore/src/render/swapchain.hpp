#pragma once

#include "render_core.hpp"

#include <queue>

namespace ENGINE_NAMESPACE
{

	class device;

	class swapchain
	{
	public:
		void init(VkPhysicalDevice& physical_handle, VkDevice& device_handle, VkSurfaceKHR& surface,
			std::uint32_t graphics_queue_idx, std::uint32_t present_queue_idx);
		void terminate(VkDevice &device_handle);
		void recreate(VkPhysicalDevice& physical_handle, VkDevice& device_handle, VkSurfaceKHR& surface,
			std::uint8_t current_frame);
		void check_destroy_old(VkDevice& device_handle, std::uint8_t current_frame);

		~swapchain();

		inline const VkSwapchainKHR& vk_handle() const noexcept { return handle; }
		inline std::uint32_t size() const noexcept { return n_images; }
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

		std::array<std::uint32_t, 2> queue_indexes;
		static const std::uint32_t render_copy = 0;
		static const std::uint32_t present = 1;

		VkImage* images = nullptr;
		std::uint32_t n_images;
		VkImageView* image_views = nullptr;

		struct old_swapchain
		{
			VkSwapchainKHR handle;
			std::uint32_t n_images;
			VkImageView* image_views;
			std::uint8_t deletion_frame;
		};

		std::queue<old_swapchain> old_swapchains;

		void update_dimensions(VkSurfaceCapabilitiesKHR& capabilities);

		friend class graphics_pipeline;
	};
}
