#include <pch.hpp>

#include <core/log.hpp>
#include <core/window.hpp>
#include <util/flow.hpp>
#include <render/vars.hpp>
#include <render/util.hpp>

#include "swapchain.hpp"

namespace hc::render::device {

    Result<InnerSwapchain, SwapchainResult> create_inner_swapchain(const VolkDeviceTable &fn_table, VkDevice device,
                                                                   const VkSwapchainCreateInfoKHR &create_info) {
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

        return Ok(InnerSwapchain{
                .handle = swapchain,
                .image_views = std::move(image_views)
        });
    }

    void destroy_inner_swapchain(const VolkDeviceTable &fn_table, VkDevice device, InnerSwapchain &swapchain) {
        if (swapchain.handle != VK_NULL_HANDLE) {
            for (VkImageView &image_view: swapchain.image_views) {
                fn_table.vkDestroyImageView(device, image_view, nullptr);
            }
            swapchain.image_views.clear();
            fn_table.vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
            swapchain.handle = VK_NULL_HANDLE;
        }
    }

    Result<Swapchain, SwapchainResult>
    Swapchain::create(const VolkDeviceTable &fn_table, VkDevice device, VkSurfaceKHR &&surface,
                      SurfaceInfo &&surface_info, SwapchainParams &&params) {
        VkSurfaceCapabilities2KHR &surface_capabilities = surface_info.capabilities;
        if (params.extent.width < surface_capabilities.surfaceCapabilities.minImageExtent.width
            || params.extent.height < surface_capabilities.surfaceCapabilities.minImageExtent.height
            || surface_capabilities.surfaceCapabilities.maxImageExtent.width < params.extent.width
            || surface_capabilities.surfaceCapabilities.maxImageExtent.height < params.extent.height) {
            HC_ERROR("Unsupported surface extent " << to_str(params.extent) << ", minimum is "
                                                   << to_str(surface_capabilities.surfaceCapabilities.minImageExtent)
                                                   << " and maximum is "
                                                   << to_str(surface_capabilities.surfaceCapabilities.maxImageExtent));
            return Err(SwapchainResult::UnsupportedSurface);
        }

        VkPresentModeKHR present_mode = surface_info.available_present_modes[0];
        VkSurfaceFormatKHR surface_format = surface_info.available_formats[0].surfaceFormat;
        u32 image_count = std::max(surface_capabilities.surfaceCapabilities.minImageCount,
                                   static_cast<u32>(max_frames_in_flight()));
        // Maximum images may be 0, indicating there is no limit
        if (surface_capabilities.surfaceCapabilities.maxImageCount > 0)
            image_count = std::min(surface_capabilities.surfaceCapabilities.maxImageCount, image_count);

        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = params.extent;
        create_info.imageArrayLayers = 1;    //usually always 1
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    //how image is used, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT when direct rendering, VK_IMAGE_USAGE_TRANSFER_DST_BIT if theres post processing

        u32 queue_families[2];
        if (params.graphics_queue == params.present_queue) {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            queue_families[0] = params.graphics_queue;
            queue_families[1] = params.present_queue;
            create_info.pQueueFamilyIndices = queue_families;
        }

        create_info.preTransform = surface_capabilities.surfaceCapabilities.currentTransform; //image transforms such as rotations or flipping, current means no tranforms applied
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //image opacity
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE; //used when it's needed to create a new target

        auto inner_res = create_inner_swapchain(fn_table, device, create_info);
        if (!inner_res)
            return Err(std::move(inner_res).err());

        Swapchain swapchain;
        swapchain.inner = std::move(inner_res).ok();
        swapchain.surface = surface;
        swapchain.surface_format = surface_format;
        swapchain.present_mode = present_mode;
        swapchain.extent = params.extent;
        swapchain.creation_params.graphics_queue_family = params.graphics_queue;
        swapchain.creation_params.present_queue_family = params.present_queue;
        swapchain.creation_params.image_count = image_count;
        swapchain.creation_params.transform = surface_capabilities.surfaceCapabilities.currentTransform;

        return Ok(std::move(swapchain));
    }

    Swapchain::~Swapchain() {
        HC_ASSERT(this->inner.handle == VK_NULL_HANDLE,
                  "Must call Swapchain::destroy before Swapchain object is destroyed");
    }

    Swapchain::Swapchain(Swapchain &&other) noexcept:
            inner(std::exchange(other.inner, {.handle = VK_NULL_HANDLE})),
            surface(std::exchange(other.surface, VK_NULL_HANDLE)),
            surface_format(std::move(other.surface_format)),
            present_mode(other.present_mode),
            extent(std::move(other.extent)),
            viewport(std::move(other.viewport)),
            scissor(std::move(other.scissor)),
            creation_params(std::move(other.creation_params)),
            presentation_fences(std::move(other.presentation_fences)),
            image_semaphores(std::move(other.image_semaphores)),
            old_swapchains(std::move(other.old_swapchains)) {}

    Swapchain &Swapchain::operator=(Swapchain &&other) noexcept {
        HC_ASSERT(this->inner.handle == VK_NULL_HANDLE, "Must call Swapchain::destroy before move assignment");

        this->inner = std::exchange(other.inner, {.handle = VK_NULL_HANDLE});
        this->surface = std::exchange(other.surface, VK_NULL_HANDLE);
        this->surface_format = std::move(other.surface_format);
        this->present_mode = other.present_mode;
        this->extent = std::move(other.extent);
        this->viewport = std::move(other.viewport);
        this->scissor = std::move(other.scissor);
        this->creation_params = std::move(other.creation_params);
        this->presentation_fences = std::move(other.presentation_fences);
        this->image_semaphores = std::move(other.image_semaphores);
        this->old_swapchains = std::move(other.old_swapchains);

        return *this;
    }

    void Swapchain::destroy(VkInstance instance, const VolkDeviceTable &fn_table, VkDevice device) {
        while (!this->old_swapchains.empty()) {
            destroy_inner_swapchain(fn_table, device, this->old_swapchains.front());
            old_swapchains.pop();
        }

        if (this->inner.handle != VK_NULL_HANDLE)
            destroy_inner_swapchain(fn_table, device, this->inner);

        vkDestroySurfaceKHR(instance, this->surface, nullptr);
    }

    void Swapchain::destroy_old(const VolkDeviceTable &fn_table, VkDevice device) {
        HC_ASSERT(!this->old_swapchains.empty(), "There should be an old swapchain to destroy");
        destroy_inner_swapchain(fn_table, device, this->old_swapchains.front());
        old_swapchains.pop();
    }

    Result<u32, SwapchainResult> Swapchain::acquire_image(const VolkDeviceTable &fn_table, VkDevice device,
                                                          GLFWwindow *window, u8 frame_mod, u64 timeout) {
        VkResult res = fn_table.vkWaitForFences(device, 1, &this->presentation_fences[frame_mod], VK_TRUE, timeout);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to wait for presentation fence: " << to_str(res));
            return Err(SwapchainResult::FenceFailure);
        }

        u32 index = std::numeric_limits<u32>::max();
        res = fn_table.vkAcquireNextImageKHR(device, this->inner.handle, timeout, image_semaphores[frame_mod],
                                             VK_NULL_HANDLE, &index);
        if (res == VK_ERROR_OUT_OF_DATE_KHR && !window::is_resizing(window)) {
            HC_DEBUG("Swapchain out of date, recreating");
            SwapchainResult swapchain_result = this->recreate(fn_table, device, window);
            if (swapchain_result != SwapchainResult::Success)
                return Err(std::move(swapchain_result));

            return Err(SwapchainResult::SkipFrame);
        } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
            HC_ERROR("Failed to acquire swapchain image: " << to_str(res));
            return Err(SwapchainResult::ImageAcquisitionFailure);
        }

        return Ok(std::move(index));
    }

    SwapchainResult Swapchain::recreate(const VolkDeviceTable &fn_table, VkDevice device, GLFWwindow *window) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        this->extent.width = static_cast<u32>(width);
        this->extent.height = static_cast<u32>(height);

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

        u32 queue_families[2];
        if (this->creation_params.graphics_queue_family == this->creation_params.present_queue_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            queue_families[0] = this->creation_params.graphics_queue_family;
            queue_families[1] = this->creation_params.present_queue_family;
            create_info.pQueueFamilyIndices = queue_families;
        }

        create_info.preTransform = this->creation_params.transform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = this->present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = this->inner.handle;

        auto inner_res = create_inner_swapchain(fn_table, device, create_info);
        if (!inner_res)
            return SwapchainResult::CreationFailure;

        this->old_swapchains.push(std::exchange(this->inner, std::move(inner_res).ok()));

        return SwapchainResult::Success;
    }
}
