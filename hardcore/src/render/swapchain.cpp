#include <pch.hpp>

#include <render/swapchain.hpp>
#include <render/device.hpp>
#include <core/client.hpp>
#include <core/window.hpp>

#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	VkSurfaceFormatKHR get_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
	{
		VkSurfaceFormatKHR res = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		u32 n_formats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &n_formats, nullptr);
		if (n_formats != 0)
		{
			VkSurfaceFormatKHR* available_formats = t_malloc<VkSurfaceFormatKHR>(n_formats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &n_formats, available_formats);
			if (n_formats == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
			{
				res = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}
			else
			{
				res = available_formats[0];
				for (u32 i = 1; i < n_formats; i++)
				{
					if (available_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && 
						available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						res = available_formats[i];
						break;
					}
				}
			}
			std::free(available_formats);
		}
		return res;
	}

	VkPresentModeKHR get_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR& surface)
	{
		VkPresentModeKHR res = VK_PRESENT_MODE_FIFO_KHR; //default
		u32 n_present_modes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &n_present_modes, nullptr);
		if (n_present_modes != 0)
		{
			VkPresentModeKHR* available_present_modes = t_malloc<VkPresentModeKHR>(n_present_modes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &n_present_modes, available_present_modes);
			res = available_present_modes[0];
			for (u32 i = 0; i < n_present_modes; i++)
			{
				if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					res = available_present_modes[i];
					break;
				}
				else if (available_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					res = available_present_modes[i];
				}
			}
			std::free(available_present_modes);
		}
		return res;
	}

	inline void swapchain::update_dimensions(VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			_extent = capabilities.currentExtent;
		}
		else
		{
			int width, height;
			window::get_dimensions(&width, &height);
			_extent = {
				static_cast<u32>(width),
				static_cast<u32>(height)
			};

			_extent.width = std::max(capabilities.minImageExtent.width, 
				std::min(capabilities.maxImageExtent.width, _extent.width));
			_extent.height = std::max(capabilities.minImageExtent.height, 
				std::min(capabilities.maxImageExtent.height, _extent.height));
		}

		_viewport = {};
		_viewport.x = 0.0f;
		_viewport.y = 0.0f;
		_viewport.width = static_cast<float>(_extent.width);
		_viewport.height = static_cast<float>(_extent.height);
		_viewport.minDepth = 0.0f;
		_viewport.maxDepth = 1.0f; //always between 1 and 0

		_scissor = {}; //all values measured in pixels
		_scissor.offset = { 0, 0 };
		_scissor.extent = _extent;
	}

	void swapchain::init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
		u32 graphics_queue_idx, u32 present_queue_idx)
	{
		//TODO ideally you can choose your preferred format ex: if you want to use HDR or not
		surface_format = get_format(physical_device, surface);

		//TODO ideally you can choose your preferred present mode ex: toggling vsync
		present_mode = get_present_mode(physical_device, surface);

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
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
				LOG_INTERNAL_INFO("[RENDERER] Present mode: FIFO Relaxed");
				break;
			}
		}

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

		update_dimensions(capabilities);

		n_images = std::max(capabilities.minImageCount, static_cast<u32>(max_frames_in_flight));
		if (capabilities.maxImageCount > 0 && n_images > capabilities.maxImageCount)
		{
			n_images = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = surface;
		create_info.minImageCount = n_images;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = _extent;
		create_info.imageArrayLayers = 1;	//usually always 1
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//how image is used, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT when direct rendering, VK_IMAGE_USAGE_TRANSFER_DST_BIT if theres post processing

		queue_indexes[render_copy] = graphics_queue_idx;
		queue_indexes[present] = present_queue_idx;
		if (queue_indexes[render_copy] != queue_indexes[present])
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = static_cast<u32>(queue_indexes.size());
			create_info.pQueueFamilyIndices = queue_indexes.data();
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		create_info.preTransform = capabilities.currentTransform; //image tranforms such as rotations or flipping, current means no tranforms applied
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE; //used when it's needed to create a new target

		VK_CRASH_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &handle), "Failed to create swapchain");
		
		vkGetSwapchainImagesKHR(device, handle, &n_images, nullptr);
		LOG_INTERNAL_INFO("Main swapchain frames: " << n_images << " ; max_frames_in_flight = " << static_cast<u32>(max_frames_in_flight));
		images = t_malloc<VkImage>(n_images);
		vkGetSwapchainImagesKHR(device, handle, &n_images, images);

		image_views = t_malloc<VkImageView>(n_images);
		for (u32 i = 0; i < n_images; i++)
		{
			VkImageViewCreateInfo view_create_info = {};
			view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_create_info.image = images[i];
			view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_create_info.format = surface_format.format;
			view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_create_info.subresourceRange.baseMipLevel = 0;
			view_create_info.subresourceRange.levelCount = 1;
			view_create_info.subresourceRange.baseArrayLayer = 0;
			view_create_info.subresourceRange.layerCount = 1;

			VK_CRASH_CHECK(vkCreateImageView(device, &view_create_info, nullptr, &image_views[i]),
				"Failed to create image view");
		}
	}

	void swapchain::terminate(VkDevice device)
	{
		while (!old_swapchains.empty()) //this shouldnt be needed, but just in case
		{
			old_swapchain& old = old_swapchains.front();
			vkDestroySwapchainKHR(device, old.handle, nullptr);
			for (u32 i = 0; i < old.n_images; i++)
				vkDestroyImageView(device, old.image_views[i], nullptr);
			std::free(old.image_views);
			old_swapchains.pop();
		}

		for (u32 i = 0; i < n_images; i++) vkDestroyImageView(device, image_views[i], nullptr);

		std::free(image_views);
		std::free(images);
		image_views = nullptr;
		images = nullptr;
		vkDestroySwapchainKHR(device, handle, nullptr);
	}

	void swapchain::recreate(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
		u8 current_frame)
	{
		old_swapchain old = {};
		old.handle = handle;
		old.n_images = n_images;
		old.image_views = image_views;
		old.deletion_frame = current_frame;

		handle = VK_NULL_HANDLE;

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

		update_dimensions(capabilities);

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = surface;
		create_info.minImageCount = n_images;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = _extent;
		create_info.imageArrayLayers = 1;	//usually always 1
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//how image is used, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT when direct rendering, VK_IMAGE_USAGE_TRANSFER_DST_BIT if theres post processing

		if (queue_indexes[render_copy] != queue_indexes[present])
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = static_cast<u32>(queue_indexes.size());
			create_info.pQueueFamilyIndices = queue_indexes.data();
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		create_info.preTransform = capabilities.currentTransform; //image tranforms such as rotations or flipping, current means no tranforms applied
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = old.handle;

		VK_CRASH_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &handle), "Failed to recreate swapchain");

		u32 new_n_images = 0;
		vkGetSwapchainImagesKHR(device, handle, &new_n_images, nullptr);
		if (new_n_images != n_images)
		{
			LOGF_INTERNAL_WARN("New swapchain uses a different number of images (old: {0}, new: {1})", 
				n_images, new_n_images);
			n_images = new_n_images;
			images = t_realloc<VkImage>(images, n_images);
		}
		vkGetSwapchainImagesKHR(device, handle, &n_images, images);

		image_views = t_malloc<VkImageView>(n_images);
		for (u32 i = 0; i < n_images; i++)
		{
			VkImageViewCreateInfo view_create_info = {};
			view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_create_info.image = images[i];
			view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_create_info.format = surface_format.format;
			view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_create_info.subresourceRange.baseMipLevel = 0;
			view_create_info.subresourceRange.levelCount = 1;
			view_create_info.subresourceRange.baseArrayLayer = 0;
			view_create_info.subresourceRange.layerCount = 1;

			VK_CRASH_CHECK(vkCreateImageView(device, &view_create_info, nullptr, &image_views[i]),
				"Failed to create image view");
		}

		old_swapchains.push(std::move(old));
	}

	void swapchain::check_destroy_old(VkDevice device, u8 current_frame)
	{
		if (!old_swapchains.empty())
		{
			old_swapchain& old = old_swapchains.front();
			if (old.deletion_frame == current_frame)
			{
				vkDestroySwapchainKHR(device, old.handle, nullptr);
				for (u32 i = 0; i < old.n_images; i++)
					vkDestroyImageView(device, old.image_views[i], nullptr);
				std::free(old.image_views);
				old_swapchains.pop();
			}
		}
	}

	swapchain::~swapchain()
	{
		INTERNAL_ASSERT(!image_views, "Swapchain was not terminated properly");
	}
}
