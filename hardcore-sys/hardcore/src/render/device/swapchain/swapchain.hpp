#pragma once

#include "swapchain_instance.hpp"

#include <core/error.hpp>
#include <core/glfw.hpp>
#include <render/vulkan.hpp>

#include <util/number.hpp>

namespace hc::render::device::swapchain {
    struct SurfaceInfo {
        VkSurfaceCapabilities2KHR capabilities = {};
        std::vector<VkSurfaceFormat2KHR> available_formats;
        std::vector<VkPresentModeKHR> available_present_modes;
    };

    struct SwapchainParams {
        VkExtent2D extent = VkExtent2D{.width = 0, .height = 0};
        VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        VkSurfaceFormatKHR preferred_format = VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
    };

    enum class AcquisitionKind : u8 {
        Normal,
        OutOfDate,
        Skip,
    };

    struct ImageDetails {
        VkSemaphore image_ready_semaphore = VK_NULL_HANDLE;
        u32 index = std::numeric_limits<u32>::max();
        AcquisitionKind acquisition = AcquisitionKind::Normal;
    };

    class Swapchain {
    public:
        Swapchain(const Swapchain&) = delete;

        Swapchain& operator=(const Swapchain&) = delete;

        [[nodiscard]] static std::expected<Swapchain, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            vk::Surface&& surface,
            SurfaceInfo&& surface_info,
            SwapchainParams&& params
        );

        Swapchain(Swapchain&& other) noexcept = default;

        Swapchain& operator=(Swapchain&& other) noexcept = default;

        void destroy(const VolkDeviceTable& fn_table, VkDevice device);

        void resize(VkExtent2D extent) noexcept;

        [[nodiscard]] std::expected<std::optional<SwapchainInstance>, Error> recreate(
            VkPhysicalDevice physical_device,
            const VolkDeviceTable& fn_table,
            VkDevice device,
            bool out_of_date
        );

        [[nodiscard]] VkSwapchainKHR handle() const noexcept { return this->current_instance.handle(); }

        [[nodiscard]] VkRenderPassBeginInfo render_pass_info(u32 image_index);

        [[nodiscard]] std::expected<ImageDetails, Error> acquire_image(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            u8 frame_mod,
            u64 timeout = std::numeric_limits<u64>::max()
        );

        void set_out_of_date() noexcept;

        [[nodiscard]] bool is_out_of_date() const noexcept { return this->images_out_of_date; }

    private:
        Swapchain() = default;

        SwapchainInstance current_instance;

        vk::Surface surface;

        VkSurfaceFormatKHR surface_format = VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

        VkExtent2D current_extent = VkExtent2D{.width = 0, .height = 0};
        VkExtent2D new_extent = VkExtent2D{.width = 0, .height = 0};
        VkViewport viewport;
        VkRect2D scissor;
        VkClearValue clear_value = {0.5f, 0.5f, 0.5f, 1.0f};
        vk::RenderPass render_pass;

        struct CreationParams {
            u32 image_count = 0;
            VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
        } creation_params;

        std::vector<vk::Semaphore> image_semaphores;

        bool images_out_of_date = false;
    };
}
