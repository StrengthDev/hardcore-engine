#include <pch.hpp>

#include "swapchain.hpp"

#include <core/log.hpp>
#include <core/window.hpp>
#include <util/flow.hpp>
#include <render/renderer.hpp>
#include <render/vars.hpp>
#include <render/util.hpp>

namespace hc::render::device {
    std::expected<InnerSwapchain, Error> InnerSwapchain::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        const VkSwapchainCreateInfoKHR& create_info,
        VkRenderPass render_pass
    ) {
        InnerSwapchain swapchain;

        VkResult result = fn_table.vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain.handle.get());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to create swapchain: " << to_str(result));
            return Error(result);
        }

        u32 image_count = 0;
        result = fn_table.vkGetSwapchainImagesKHR(device, swapchain.handle, &image_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query swapchain images: " << to_str(result));
            swapchain.destroy(fn_table, device);
            return Error(result);
        }

        std::vector<VkImage> images(image_count);
        result = fn_table.vkGetSwapchainImagesKHR(device, swapchain.handle, &image_count, images.data());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to obtain swapchain images: " << to_str(result));
            swapchain.destroy(fn_table, device);
            return Error(result);
        }

        swapchain.image_views.reserve(image_count);
        for (u32 i = 0; i < image_count; i++) {
            VkImageViewCreateInfo view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = create_info.imageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            VkImageView image_view = VK_NULL_HANDLE;
            result = fn_table.vkCreateImageView(device, &view_create_info, nullptr, &image_view);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create swapchain image view: " << to_str(result));
                swapchain.destroy(fn_table, device);
                return Error(result);
            }

            swapchain.image_views.push_back(image_view);
        }

        swapchain.framebuffers.reserve(image_count);
        for (u32 i = 0; i < image_count; i++) {
            VkFramebufferCreateInfo framebuffer_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = render_pass,
                .attachmentCount = 1,
                .pAttachments = &swapchain.image_views[i].get(),
                .width = create_info.imageExtent.width,
                .height = create_info.imageExtent.height,
                .layers = 1,
            };

            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            result = fn_table.vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create swapchain frame buffer: " << to_str(result));
                swapchain.destroy(fn_table, device);
                return Error(result);
            }

            swapchain.framebuffers.push_back(framebuffer);
        }

        return swapchain;
    }

    void InnerSwapchain::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
        for (auto& framebuffer : this->framebuffers) {
            fn_table.vkDestroyFramebuffer(device, framebuffer, nullptr);
            framebuffer.destroy();
        }
        this->framebuffers.clear();

        for (auto& image_view : this->image_views) {
            fn_table.vkDestroyImageView(device, image_view, nullptr);
            image_view.destroy();
        }
        this->image_views.clear();

        fn_table.vkDestroySwapchainKHR(device, this->handle, nullptr);
        this->handle.destroy();
    }

    std::expected<Swapchain, Error> Swapchain::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE>&& surface,
        SurfaceInfo&& surface_info,
        SwapchainParams&& params
    ) {
        Swapchain swapchain;

        VkSurfaceCapabilities2KHR& surface_capabilities = surface_info.capabilities;
        params.extent.width = std::clamp(
            params.extent.width,
            surface_capabilities.surfaceCapabilities.minImageExtent.width,
            surface_capabilities.surfaceCapabilities.maxImageExtent.width
        );

        params.extent.height = std::clamp(
            params.extent.height,
            surface_capabilities.surfaceCapabilities.minImageExtent.height,
            surface_capabilities.surfaceCapabilities.maxImageExtent.height
        );

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

        swapchain.surface = std::move(surface);
        swapchain.surface_format = surface_format;
        swapchain.present_mode = present_mode;
        swapchain.extent = params.extent;
        swapchain.creation_params.image_count = image_count;
        swapchain.creation_params.transform = surface_capabilities.surfaceCapabilities.currentTransform;

        VkAttachmentDescription attachment = {
            .flags = 0,
            .format = surface_format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference attachment_ref = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachment_ref,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        };

        VkRenderPassCreateInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
        };

        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkResult result = fn_table.vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to create swapchain render pass: " << to_str(result));
            swapchain.destroy(fn_table, device);
            return Error(result);
        }

        swapchain.render_pass = render_pass;

        VkSwapchainCreateInfoKHR create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = swapchain.surface,
            .minImageCount = image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = params.extent,
            // usually always 1
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            // image transforms such as rotations or flipping, current means no transforms applied
            .preTransform = surface_capabilities.surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };

        auto inner_result = InnerSwapchain::create(fn_table, device, create_info, render_pass);
        if (!inner_result) {
            swapchain.destroy(fn_table, device);
            return inner_result.error();
        }

        swapchain.inner = *std::move(inner_result);

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };

        swapchain.image_semaphores.reserve(max_frames_in_flight());
        for (u8 i = 0; i < max_frames_in_flight(); ++i) {
            VkSemaphore semaphore = VK_NULL_HANDLE;
            result = fn_table.vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore);

            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create swapchain image semaphore: " << to_str(result));
                swapchain.destroy(fn_table, device);
                return Error(result);
            }

            swapchain.image_semaphores.push_back(semaphore);
        }

        return swapchain;
    }

    Swapchain::~Swapchain() {
        HC_ASSERT(
            this->inner.handle == VK_NULL_HANDLE,
            "Must call Swapchain::destroy before Swapchain object is destroyed"
        );
    }

    void Swapchain::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
        for (auto& semaphore : this->image_semaphores) {
            fn_table.vkDestroySemaphore(device, semaphore, nullptr);
            semaphore.destroy();
        }
        this->image_semaphores.clear();

        while (!this->old_swapchains.empty()) {
            this->old_swapchains.front().destroy(fn_table, device);
            old_swapchains.pop();
        }

        if (this->inner.handle.valid()) {
            this->inner.destroy(fn_table, device);
        }

        if (this->render_pass.valid()) {
            fn_table.vkDestroyRenderPass(device, this->render_pass, nullptr);
            this->render_pass.destroy();
        }

        if (this->surface.valid()) {
            vkDestroySurfaceKHR(vk_instance(), this->surface, nullptr);
            this->surface.destroy();
        }
    }

    std::expected<bool, Error> Swapchain::recreate(
        VkPhysicalDevice physical_device,
        const VolkDeviceTable& fn_table,
        VkDevice device,
        GLFWwindow* window,
        bool out_of_date
    ) {
        this->images_out_of_date = out_of_date;

        // if (window::is_resizing(window)) {
        //     return false;
        // }

        this->extent = window::extent(window);

        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext = nullptr,
            .surface = this->surface,
        };

        VkSurfaceCapabilities2KHR capabilities = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
            .pNext = nullptr,
            .surfaceCapabilities = {},
        };
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &capabilities);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface capabilities: " << to_str(result));
            return Error(result);
        }

        this->extent.width = std::clamp(
            this->extent.width,
            capabilities.surfaceCapabilities.minImageExtent.width,
            capabilities.surfaceCapabilities.maxImageExtent.width
        );

        this->extent.height = std::clamp(
            this->extent.height,
            capabilities.surfaceCapabilities.minImageExtent.height,
            capabilities.surfaceCapabilities.maxImageExtent.height
        );

        VkSwapchainCreateInfoKHR create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = this->surface,
            .minImageCount = this->creation_params.image_count,
            .imageFormat = this->surface_format.format,
            .imageColorSpace = this->surface_format.colorSpace,
            .imageExtent = this->extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = this->creation_params.transform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = this->present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = this->inner.handle,
        };

        auto inner_result = InnerSwapchain::create(fn_table, device, create_info, this->render_pass);
        if (!inner_result) {
            return inner_result.error();
        }

        this->old_swapchains.push(std::exchange(this->inner, *std::move(inner_result)));

        this->images_out_of_date = false;

        HC_DEBUG("Swapchain recreated with size (" << this->extent.width << ", " << this->extent.height << ')');

        return true;
    }

    void Swapchain::destroy_old(const VolkDeviceTable& fn_table, VkDevice device) {
        HC_ASSERT(!this->old_swapchains.empty(), "There should be an old swapchain to destroy");
        this->old_swapchains.front().destroy(fn_table, device);
        old_swapchains.pop();
    }

    VkRenderPassBeginInfo Swapchain::render_pass_info(u32 image_index) {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = this->render_pass,
            .framebuffer = this->inner.framebuffers[image_index],
            .renderArea = {
                .offset = {0, 0},
                .extent = this->extent,
            },
            .clearValueCount = 1,
            .pClearValues = &this->clear_value,
        };
    }

    std::expected<ImageDetails, Error> Swapchain::acquire_image(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        u8 frame_mod,
        u64 timeout
    ) {
        if (this->images_out_of_date) {
            return ImageDetails{.acquisition = AcquisitionKind::OutOfDate};
        }

        u32 index = std::numeric_limits<u32>::max();
        VkSemaphore semaphore = image_semaphores[frame_mod];
        VkResult result = fn_table.vkAcquireNextImageKHR(
            device,
            this->inner.handle,
            timeout,
            semaphore,
            VK_NULL_HANDLE,
            &index
        );

        switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            return ImageDetails{
                .image_ready_semaphore = semaphore,
                .index = index,
                .acquisition = AcquisitionKind::Normal
            };
        case VK_ERROR_OUT_OF_DATE_KHR:
            return ImageDetails{.acquisition = AcquisitionKind::OutOfDate};
        case VK_TIMEOUT:
        case VK_NOT_READY: // Returned when timeout is 0 and image is not ready
            return ImageDetails{.acquisition = AcquisitionKind::Skip};
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            HC_ERROR("Failed to acquire swapchain image: " << to_str(result));
            return Error(result);
        default: HC_UNREACHABLE("No other errors should be returned by vkAcquireNextImageKHR");
        }
    }

    void Swapchain::set_out_of_date() noexcept {
        this->images_out_of_date = true;
    }
}
