
#pragma once

#include <core/error.hpp>
#include <util/uncopyable.hpp>
#include <util/number.hpp>

#include <vulkan/vulkan.h>

namespace hc::render::device::swapchain {
    class SwapchainInstance {
    public:
        [[nodiscard]] static std::expected<SwapchainInstance, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            const VkSwapchainCreateInfoKHR& create_info,
            VkRenderPass render_pass
        );

        void destroy(const VolkDeviceTable& fn_table, VkDevice device);

        [[nodiscard]] VkSwapchainKHR handle() const noexcept { return this->swapchain_handle; }

        [[nodiscard]] VkFramebuffer framebuffer(u32 index) const { return this->framebuffers[index]; }

    private:
        ExternalHandle<VkSwapchainKHR, VK_NULL_HANDLE> swapchain_handle;
        std::vector<ExternalHandle<VkImageView, VK_NULL_HANDLE>> image_views;
        std::vector<ExternalHandle<VkFramebuffer, VK_NULL_HANDLE>> framebuffers;
    };
}
