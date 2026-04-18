
#pragma once

#include "../../vulkan.hpp"

#include <core/error.hpp>
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
        vk::Swapchain swapchain_handle;
        std::vector<vk::ImageView> image_views;
        std::vector<vk::Framebuffer> framebuffers;
    };
}
