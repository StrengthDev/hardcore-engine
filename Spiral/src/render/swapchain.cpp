#include <pch.hpp>

#include <spiral/render/swapchain.hpp>
#include <spiral/core/client.hpp>
#include <spiral/core/window.hpp>

namespace Spiral
{
	void Swapchain::init(VkPhysicalDevice physicalHandle, VkDevice logicalHandle, VkSurfaceKHR surface, uint32_t graphicsIndex, uint32_t presentIndex)
	{
		uint32_t i;
		int width, height;
		VkSurfaceFormatKHR surfaceFormat;
		VkSurfaceFormatKHR* availableFormats;
		uint32_t nFormats;
		VkPresentModeKHR presentMode;
		VkPresentModeKHR* availablePresentModes;
		uint32_t nPresentModes;
		VkSurfaceCapabilitiesKHR capabilities;
		valid = true;

		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalHandle, surface, &nFormats, nullptr);
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		if (nFormats != 0)
		{
			availableFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * nFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalHandle, surface, &nFormats, availableFormats);
			if (nFormats == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}
			else
			{
				surfaceFormat = availableFormats[0];
				for (i = 0; i < nFormats; i++)
				{
					if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						surfaceFormat = availableFormats[i];
						break;
					}
				}
			}
			free(availableFormats);
		}

		presentMode = VK_PRESENT_MODE_FIFO_KHR;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalHandle, surface, &nPresentModes, nullptr);
		if (nPresentModes != 0)
		{
			availablePresentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * nPresentModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalHandle, surface, &nPresentModes, availablePresentModes);
			for (i = 0; i < nPresentModes; i++)
			{
				if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					presentMode = availablePresentModes[i];
					break;
				}
				else if (availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					presentMode = availablePresentModes[i];
				}
			}
			free(availablePresentModes);
		}

		if (enableValidationLayers)
		{
			switch (presentMode)
			{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:
				LOG_INTERNAL_DEBUG("[RENDERER] Present mode: Immediate");
				break;
			case VK_PRESENT_MODE_MAILBOX_KHR:
				LOG_INTERNAL_DEBUG("[RENDERER] Present mode: Mailbox");
				break;
			case VK_PRESENT_MODE_FIFO_KHR:
				LOG_INTERNAL_DEBUG("[RENDERER] Present mode: FIFO");
				break;
			}
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalHandle, surface, &capabilities);
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			extent = capabilities.currentExtent;
		}
		else
		{
			window::get_dimensions(&width, &height);
			extent = {
				(uint32_t)width,
				(uint32_t)height
			};

			extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
			extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		}

		nImages = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && nImages > capabilities.maxImageCount)
		{
			nImages = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = nImages;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;	//usually always 1
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//how image is used, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT when direct rendering, VK_IMAGE_USAGE_TRANSFER_DST_BIT if theres post processing

		uint32_t queueFamilyIndices[] = { graphicsIndex, presentIndex };
		if (graphicsIndex != presentIndex)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = capabilities.currentTransform; //image tranforms such as rotations or flipping, current means no tranforms applied
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE; //used when it's needed to create a new swapchain

		if (vkCreateSwapchainKHR(logicalHandle, &createInfo, nullptr, &handle) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			return;
		}

		vkGetSwapchainImagesKHR(logicalHandle, handle, &nImages, nullptr);
		images = (VkImage*)malloc(sizeof(VkImage) * nImages);
		vkGetSwapchainImagesKHR(logicalHandle, handle, &nImages, images);

		imageFormat = surfaceFormat.format;

		imageViews = (VkImageView*)malloc(sizeof(VkImageView) * nImages);
		for (i = 0; i < nImages; i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = imageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(logicalHandle, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
			{
				DEBUG_BREAK;
				valid = false;
				free(imageViews); //TODO: destroy image views that might have already been created
				free(images);
				vkDestroySwapchainKHR(logicalHandle, handle, nullptr);
				return;
			}
		}
	}

	void Swapchain::terminate(VkDevice logicalHandle)
	{
		uint32_t i;
		for (i = 0; i < nImages; i++)
		{
			vkDestroyImageView(logicalHandle, imageViews[i], nullptr);
		}

		free(imageViews);
		free(images);
		vkDestroySwapchainKHR(logicalHandle, handle, nullptr);
	}
}