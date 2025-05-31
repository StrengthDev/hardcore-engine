#include <pch.hpp>

#include "swapchain.hpp"

#include <core/log.hpp>
#include <core/window.hpp>
#include <util/flow.hpp>
#include <render/vars.hpp>
#include <render/util.hpp>

namespace hc::render::device {
    Result<InnerSwapchain, SwapchainResult> create_inner_swapchain(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        const VkSwapchainCreateInfoKHR& create_info
    ) {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkResult res = fn_table.vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to create swapchain: " << to_str(res));
            return Err(SwapchainResult::CreationFailure);
        }

        u32 image_count = 0;
        res = fn_table.vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query swapchain images: " << to_str(res));
            fn_table.vkDestroySwapchainKHR(device, swapchain, nullptr);
            return Err(SwapchainResult::ImageAcquisitionFailure);
        }
        std::vector<VkImage> images(image_count);
        res = fn_table.vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to obtain swapchain images: " << to_str(res));
            fn_table.vkDestroySwapchainKHR(device, swapchain, nullptr);
            return Err(SwapchainResult::ImageAcquisitionFailure);
        }

        std::vector<VkImageView> image_views(image_count);
        for (u32 i = 0; i < image_count; i++) {
            VkImageViewCreateInfo view_create_info = {};
            view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create_info.image = images[i];
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.format = create_info.imageFormat;
            view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            res = fn_table.vkCreateImageView(device, &view_create_info, nullptr, &image_views[i]);
            if (res != VK_SUCCESS) {
                HC_ERROR("Failed to create swapchain image view: " << to_str(res));
                for (u32 j = 0; j < i; j++) {
                    fn_table.vkDestroyImageView(device, image_views[j], nullptr);
                }
                fn_table.vkDestroySwapchainKHR(device, swapchain, nullptr);
                return Err(SwapchainResult::ImageViewFailure);
            }
        }

        return Ok(InnerSwapchain{.handle = swapchain, .image_views = std::move(image_views)});
    }

    void destroy_inner_swapchain(const VolkDeviceTable& fn_table, VkDevice device, InnerSwapchain& swapchain) {
        if (swapchain.handle != VK_NULL_HANDLE) {
            for (VkImageView& image_view : swapchain.image_views) {
                fn_table.vkDestroyImageView(device, image_view, nullptr);
            }
            swapchain.image_views.clear();
            fn_table.vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
            swapchain.handle.destroy();
        }
    }

    Result<Swapchain, SwapchainResult> Swapchain::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkSurfaceKHR&& surface,
        SurfaceInfo&& surface_info,
        SwapchainParams&& params
    ) {
        VkSurfaceCapabilities2KHR& surface_capabilities = surface_info.capabilities;
        if (params.extent.width < surface_capabilities.surfaceCapabilities.minImageExtent.width
            || params.extent.height < surface_capabilities.surfaceCapabilities.minImageExtent.height
            || surface_capabilities.surfaceCapabilities.maxImageExtent.width < params.extent.width
            || surface_capabilities.surfaceCapabilities.maxImageExtent.height < params.extent.height) {
            HC_ERROR(
                "Unsupported surface extent " << to_str(params.extent) << ", minimum is " << to_str(surface_capabilities
                    .surfaceCapabilities.minImageExtent) << " and maximum is " << to_str(surface_capabilities.
                    surfaceCapabilities.maxImageExtent)
            );
            return Err(SwapchainResult::UnsupportedSurface);
        }

        VkPresentModeKHR present_mode = surface_info.available_present_modes[0];
        VkSurfaceFormatKHR surface_format = surface_info.available_formats[0].surfaceFormat;
        u32 image_count = std::max(
            surface_capabilities.surfaceCapabilities.minImageCount,
            static_cast<u32>(max_frames_in_flight())
        );
        // Maximum images may be 0, indicating there is no limit
        if (surface_capabilities.surfaceCapabilities.maxImageCount > 0) {
            image_count = std::min(
                surface_capabilities.surfaceCapabilities.maxImageCount,
                image_count
            );
        }

        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = params.extent;
        create_info.imageArrayLayers = 1; //usually always 1
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
        //image transforms such as rotations or flipping, current means no transforms applied
        create_info.preTransform = surface_capabilities.surfaceCapabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE; //used when it's needed to create a new target

        auto inner_res = create_inner_swapchain(fn_table, device, create_info);
        if (!inner_res) {
            return Err(std::move(inner_res).err());
        }

        Swapchain swapchain;
        swapchain.inner = std::move(inner_res).ok();
        swapchain.surface = surface;
        swapchain.surface_format = surface_format;
        swapchain.present_mode = present_mode;
        swapchain.extent = params.extent;
        swapchain.creation_params.image_count = image_count;
        swapchain.creation_params.transform = surface_capabilities.surfaceCapabilities.currentTransform;

        return Ok(std::move(swapchain));
    }

    Swapchain::~Swapchain() {
        HC_ASSERT(
            this->inner.handle == VK_NULL_HANDLE,
            "Must call Swapchain::destroy before Swapchain object is destroyed"
        );
    }

    void Swapchain::destroy(VkInstance instance, const VolkDeviceTable& fn_table, VkDevice device) {
        while (!this->old_swapchains.empty()) {
            destroy_inner_swapchain(fn_table, device, this->old_swapchains.front());
            old_swapchains.pop();
        }

        if (this->inner.handle != VK_NULL_HANDLE)
            destroy_inner_swapchain(fn_table, device, this->inner);

        vkDestroySurfaceKHR(instance, this->surface, nullptr);
        this->surface.destroy();
    }

    void Swapchain::destroy_old(const VolkDeviceTable& fn_table, VkDevice device) {
        HC_ASSERT(!this->old_swapchains.empty(), "There should be an old swapchain to destroy");
        destroy_inner_swapchain(fn_table, device, this->old_swapchains.front());
        old_swapchains.pop();
    }

    std::expected<u32, SwapchainResult> Swapchain::acquire_image(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        GLFWwindow* window,
        u8 frame_mod,
        u64 timeout
    ) {
        VkResult res = fn_table.vkWaitForFences(device, 1, &this->presentation_fences[frame_mod], VK_TRUE, timeout);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to wait for presentation fence: " << to_str(res));
            return std::unexpected(SwapchainResult::FenceFailure);
        }

        u32 index = std::numeric_limits<u32>::max();
        res = fn_table.vkAcquireNextImageKHR(
            device,
            this->inner.handle,
            timeout,
            image_semaphores[frame_mod],
            VK_NULL_HANDLE,
            &index
        );

        switch (res) {
        case VK_SUBOPTIMAL_KHR:
            if (!window::is_resizing(window)) {
                HC_DEBUG("Suboptimal swapchain, recreating..");
                SwapchainResult swapchain_result = this->recreate(fn_table, device, window);
                if (swapchain_result != SwapchainResult::Success) {
                    return std::unexpected(std::move(swapchain_result));
                }
            }
        case VK_SUCCESS:
            return index;
        case VK_ERROR_OUT_OF_DATE_KHR:
            if (!window::is_resizing(window)) {
                HC_DEBUG("Swapchain out of date, recreating..");
                SwapchainResult swapchain_result = this->recreate(fn_table, device, window);
                if (swapchain_result != SwapchainResult::Success) {
                    return std::unexpected(std::move(swapchain_result));
                }
            }
        case VK_TIMEOUT:
        case VK_NOT_READY: // Returned when timeout is 0 and image is not ready
            return std::unexpected(SwapchainResult::SkipFrame);
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return std::unexpected(SwapchainResult::ImageAcquisitionFailure);
        default: HC_UNREACHABLE("No other errors should be returned by vkAcquireNextImageKHR");
        }
    }

    SwapchainResult Swapchain::recreate(const VolkDeviceTable& fn_table, VkDevice device, GLFWwindow* window) {
        this->extent = window::extent(window);

        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.surface = this->surface;
        create_info.minImageCount = this->creation_params.image_count;
        create_info.imageFormat = this->surface_format.format;
        create_info.imageColorSpace = this->surface_format.colorSpace;
        create_info.imageExtent = this->extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
        create_info.preTransform = this->creation_params.transform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = this->present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = this->inner.handle;

        auto inner_res = create_inner_swapchain(fn_table, device, create_info);
        if (!inner_res) {
            HC_ERROR("Failed to recreate swapchain");
            return SwapchainResult::CreationFailure;
        }

        this->old_swapchains.push(std::exchange(this->inner, std::move(inner_res).ok()));

        return SwapchainResult::Success;
    }
}
