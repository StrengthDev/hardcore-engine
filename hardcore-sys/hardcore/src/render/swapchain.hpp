#pragma once

#include <optional>
#include <queue>

#include <volk.h>

#include <core/util.hpp>

namespace hc::render {
    class Device;

    struct SurfaceInfo {
        VkSurfaceCapabilities2KHR capabilities = {};
        std::vector<VkSurfaceFormat2KHR> available_formats;
        std::vector<VkPresentModeKHR> available_present_modes;
    };

    struct SwapchainParams {
        u32 graphics_queue = std::numeric_limits<u32>::max();
        u32 present_queue = std::numeric_limits<u32>::max();
        VkExtent2D extent = VkExtent2D{.width = 0, .height = 0};
        VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        VkSurfaceFormatKHR preferred_format = VkSurfaceFormatKHR{
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
    };

    struct InnerSwapchain {
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        std::vector<VkImageView> image_views;
    };

    class Swapchain {
    public:
        Swapchain(const Swapchain &) = delete;

        Swapchain &operator=(const Swapchain &) = delete;

        static std::optional<Swapchain>
        create(const VolkDeviceTable &fn_table, VkDevice device, VkSurfaceKHR &&surface, SurfaceInfo &&surface_info,
               SwapchainParams params);

        ~Swapchain();

        Swapchain(Swapchain &&other) noexcept;

        Swapchain &operator=(Swapchain &&other) noexcept;

        void destroy(VkInstance instance, const VolkDeviceTable &fn_table, VkDevice device);

        void destroy_old(const VolkDeviceTable &fn_table, VkDevice device);


    private:
        Swapchain() = default;

        InnerSwapchain inner;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surface_format = VkSurfaceFormatKHR{.format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

        VkExtent2D extent = VkExtent2D{.width = 0, .height = 0};
        VkViewport viewport;
        VkRect2D scissor;

        std::queue<InnerSwapchain> old_swapchains;
    };
}
