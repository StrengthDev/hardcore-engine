#pragma once

#include "RenderCore.hpp"

namespace Spiral
{
	struct Swapchain
	{
		VkSwapchainKHR handle;
		VkImage* swapchainImages;
		uint32_t nImages;
		VkImageView* swapchainImageViews;
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;

		VkResult creation;

		void init(VkPhysicalDevice physicalHandle, VkDevice logicalHandle, VkSurfaceKHR surface, uint32_t graphicsIndex, uint32_t presentIndex);
		void terminate(VkDevice logicalHandle);
	};
}