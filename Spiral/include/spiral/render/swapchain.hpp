#pragma once

#include "render_core.hpp"

namespace ENGINE_NAMESPACE
{

	class device;

	class swapchain
	{
	public:
		~swapchain();

		void init(device& owner);
		void terminate();

	private:
		VkSwapchainKHR handle;
		VkImage* images = nullptr;
		uint32_t n_images;
		VkImageView* image_views = nullptr;
		VkFormat image_format;
		VkExtent2D extent;

		device* owner = nullptr;

		friend device;
		friend class graphics_pipeline;
	};
}
