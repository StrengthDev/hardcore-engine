#include <pch.hpp>

#include "swapchain.hpp"

#include <core/log.hpp>
#include <render/renderer.hpp>
#include <render/util.hpp>
#include <render/vars.hpp>

#include <util/flow.hpp>

namespace hc::render::device::swapchain {
    std::expected<Swapchain, Error> Swapchain::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        vk::Surface&& surface,
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
        swapchain.current_extent = params.extent;
        swapchain.new_extent = params.extent;
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

        auto render_pass_result = vk::RenderPass::create(fn_table, device, &render_pass_info);
        if (!render_pass_result) {
            swapchain.destroy(fn_table, device);
            return render_pass_result.error();
        }
        swapchain.render_pass = *std::move(render_pass_result);

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

        auto instance_result = SwapchainInstance::create(fn_table, device, create_info, swapchain.render_pass);
        if (!instance_result) {
            swapchain.destroy(fn_table, device);
            return instance_result.error();
        }

        swapchain.current_instance = *std::move(instance_result);

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };

        swapchain.image_semaphores.reserve(max_frames_in_flight());
        for (u8 i = 0; i < max_frames_in_flight(); ++i) {
            auto semaphore_result = vk::Semaphore::create(fn_table, device, &semaphore_info);
            if (!semaphore_result) {
                swapchain.destroy(fn_table, device);
                return semaphore_result.error();
            }

            swapchain.image_semaphores.emplace_back(*std::move(semaphore_result));
        }

        return swapchain;
    }

    void Swapchain::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
        for (auto& semaphore : this->image_semaphores) {
            semaphore.destroy(fn_table, device);
        }
        this->image_semaphores.clear();

        this->current_instance.destroy(fn_table, device);
        this->render_pass.destroy(fn_table, device);
        this->surface.destroy(vk_instance());
    }

    void Swapchain::resize(VkExtent2D extent) noexcept {
        this->new_extent = extent;

        this->images_out_of_date = !(this->current_extent.width == extent.width && this->current_extent.height == extent.height);
    }

    std::expected<std::optional<SwapchainInstance>, Error> Swapchain::recreate(
        VkPhysicalDevice physical_device,
        const VolkDeviceTable& fn_table,
        VkDevice device,
        bool out_of_date
    ) {
        this->images_out_of_date = out_of_date;

        // Disable swapchain recreation while window is resizing
        // if (window::is_resizing(window)) {
        //     return std::nullopt;
        // }

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

        this->new_extent.width = std::clamp(
            this->new_extent.width,
            capabilities.surfaceCapabilities.minImageExtent.width,
            capabilities.surfaceCapabilities.maxImageExtent.width
        );

        this->new_extent.height = std::clamp(
            this->new_extent.height,
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
            .imageExtent = this->new_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = this->creation_params.transform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = this->present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = this->current_instance.handle(),
        };

        auto instance_result = SwapchainInstance::create(fn_table, device, create_info, this->render_pass);
        if (!instance_result) {
            return instance_result.error();
        }

        this->images_out_of_date = false;
        this->current_extent = this->new_extent;

        HC_DEBUG("Swapchain recreated with size (" << this->current_extent.width << ", " << this->current_extent.height << ')');

        return std::exchange(this->current_instance, *std::move(instance_result));
    }

    VkRenderPassBeginInfo Swapchain::render_pass_info(u32 image_index) {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = this->render_pass,
            .framebuffer = this->current_instance.framebuffer(image_index),
            .renderArea = {
                .offset = {0, 0},
                .extent = this->current_extent,
            },
            .clearValueCount = 1,
            .pClearValues = &this->clear_value,
            // TODO: the variant of the clear color union depends on the format of the texture, the surface format in this case
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
            this->current_instance.handle(),
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
