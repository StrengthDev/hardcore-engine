
#include "swapchain_instance.hpp"

#include "../../util.hpp"

namespace hc::render::device::swapchain {
    std::expected<SwapchainInstance, Error> SwapchainInstance::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        const VkSwapchainCreateInfoKHR& create_info,
        VkRenderPass render_pass
    ) {
        SwapchainInstance swapchain;

        VkResult result = fn_table.vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain.swapchain_handle.get());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to create swapchain: " << to_str(result));
            return Error(result);
        }

        u32 image_count = 0;
        result = fn_table.vkGetSwapchainImagesKHR(device, swapchain.swapchain_handle, &image_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query swapchain images: " << to_str(result));
            swapchain.destroy(fn_table, device);
            return Error(result);
        }

        std::vector<VkImage> images(image_count);
        result = fn_table.vkGetSwapchainImagesKHR(device, swapchain.swapchain_handle, &image_count, images.data());
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

            swapchain.image_views.emplace_back(image_view);
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

            swapchain.framebuffers.emplace_back(framebuffer);
        }

        return swapchain;
    }

    void SwapchainInstance::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
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

        if (this->swapchain_handle.valid()) {
            fn_table.vkDestroySwapchainKHR(device, this->swapchain_handle, nullptr);
            this->swapchain_handle.destroy();
        }
    }
}
