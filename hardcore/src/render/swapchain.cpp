#include <pch.hpp>

#include <render/swapchain.hpp>
#include <render/device.hpp>
#include <core/client.hpp>
#include <core/window.hpp>

#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	swapchain::~swapchain()
	{
		if (owner)
			terminate();
	}

	void swapchain::init(device& owner)
	{
		uint32_t i;
		this->owner = &owner;

		uint32_t n_formats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(owner.physical_handle, *owner.surface, &n_formats, nullptr);
		VkSurfaceFormatKHR surface_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		if (n_formats != 0)
		{
			VkSurfaceFormatKHR* available_formats = t_malloc<VkSurfaceFormatKHR>(n_formats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(owner.physical_handle, *owner.surface, &n_formats, available_formats);
			if (n_formats == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
			{
				surface_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}
			else
			{
				surface_format = available_formats[0];
				for (i = 0; i < n_formats; i++)
				{
					if (available_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						surface_format = available_formats[i];
						break;
					}
				}
			}
			std::free(available_formats);
		}

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; //default
		uint32_t n_present_modes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(owner.physical_handle, *owner.surface, &n_present_modes, nullptr);
		if (n_present_modes != 0)
		{
			VkPresentModeKHR* available_present_modes = t_malloc<VkPresentModeKHR>(n_present_modes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(owner.physical_handle, *owner.surface, &n_present_modes, available_present_modes);
			for (i = 0; i < n_present_modes; i++)
			{
				if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					present_mode = available_present_modes[i];
					break;
				}
				else if (available_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					present_mode = available_present_modes[i];
				}
			}
			std::free(available_present_modes);
		}

		if (enable_validation_layers)
		{
			switch (present_mode)
			{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:
				LOG_INTERNAL_INFO("[RENDERER] Present mode: Immediate");
				break;
			case VK_PRESENT_MODE_MAILBOX_KHR:
				LOG_INTERNAL_INFO("[RENDERER] Present mode: Mailbox");
				break;
			case VK_PRESENT_MODE_FIFO_KHR:
				LOG_INTERNAL_INFO("[RENDERER] Present mode: FIFO");
				break;
			}
		}

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(owner.physical_handle, *owner.surface, &capabilities);
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			extent = capabilities.currentExtent;
		}
		else
		{
			int width, height;
			window::get_dimensions(&width, &height);
			extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
			extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		}

		n_images = std::max(capabilities.minImageCount + 1, static_cast<uint32_t>(max_frames_in_flight));
		if (capabilities.maxImageCount > 0 && n_images > capabilities.maxImageCount)
		{
			n_images = capabilities.maxImageCount;
		}
		LOG_INTERNAL_INFO("Main swapchain frames: " << n_images << " ; max_frames_in_flight = " << static_cast<uint32_t>(max_frames_in_flight))

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = *owner.surface;
		create_info.minImageCount = n_images;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1;	//usually always 1
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//how image is used, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT when direct rendering, VK_IMAGE_USAGE_TRANSFER_DST_BIT if theres post processing

		std::uint32_t queue_family_indexes[] = { owner.graphics_idx, owner.present_idx };
		if (owner.graphics_idx != owner.present_idx)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indexes;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0; // Optional
			create_info.pQueueFamilyIndices = nullptr; // Optional
		}

		create_info.preTransform = capabilities.currentTransform; //image tranforms such as rotations or flipping, current means no tranforms applied
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE; //used when it's needed to create a new target

		if (vkCreateSwapchainKHR(owner.handle, &create_info, nullptr, &handle) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			shutdown();
			return;
		}

		vkGetSwapchainImagesKHR(owner.handle, handle, &n_images, nullptr);
		images = t_malloc<VkImage>(n_images);
		vkGetSwapchainImagesKHR(owner.handle, handle, &n_images, images);

		image_format = surface_format.format;

		image_views = t_malloc<VkImageView>(n_images);
		for (i = 0; i < n_images; i++)
		{
			VkImageViewCreateInfo view_create_info = {};
			view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_create_info.image = images[i];
			view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_create_info.format = image_format;
			view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_create_info.subresourceRange.baseMipLevel = 0;
			view_create_info.subresourceRange.levelCount = 1;
			view_create_info.subresourceRange.baseArrayLayer = 0;
			view_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(owner.handle, &view_create_info, nullptr, &image_views[i]) != VK_SUCCESS)
			{
				DEBUG_BREAK;
				shutdown();
				/*
				valid = false;
				std::free(image_views); //TODO: destroy image views that might have already been created
				std::free(images);
				vkDestroySwapchainKHR(owner.handle, handle, nullptr);
				*/
				return;
			}
		}
	}

	void swapchain::terminate()
	{
		for (uint32_t i = 0; i < n_images; i++)
		{
			vkDestroyImageView(owner->handle, image_views[i], nullptr);
		}

		std::free(image_views);
		std::free(images);
		vkDestroySwapchainKHR(owner->handle, handle, nullptr);
		owner = nullptr;
	}
}
