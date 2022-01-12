#pragma once

#include "render_core.hpp"

namespace Spiral
{
	struct Swapchain
	{
		VkSwapchainKHR handle;
		VkImage* images;
		uint32_t nImages;
		VkImageView* imageViews;
		VkFormat imageFormat;
		VkExtent2D extent;

		bool valid;

		void init(VkPhysicalDevice physicalHandle, VkDevice logicalHandle, VkSurfaceKHR surface, uint32_t graphicsIndex, uint32_t presentIndex);
		void terminate(VkDevice logicalHandle);
	};
}