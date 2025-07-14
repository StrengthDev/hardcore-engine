#pragma once

#include <core/error.hpp>
#include <core/glfw.hpp>

#include <util/number.hpp>
#include <util/uncopyable.hpp>

#include <queue>

namespace hc::render::device {
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

    struct InnerSwapchain {
        ExternalHandle<VkSwapchainKHR, VK_NULL_HANDLE> handle;
        std::vector<ExternalHandle<VkImageView, VK_NULL_HANDLE>> image_views;
        std::vector<ExternalHandle<VkFramebuffer, VK_NULL_HANDLE>> framebuffers;

        static std::expected<InnerSwapchain, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            const VkSwapchainCreateInfoKHR& create_info,
            VkRenderPass render_pass
        );

        void destroy(const VolkDeviceTable& fn_table, VkDevice device);
    };

    class Swapchain {
    public:
        Swapchain(const Swapchain&) = delete;

        Swapchain& operator=(const Swapchain&) = delete;

        [[nodiscard]] static std::expected<Swapchain, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE>&& surface,
            SurfaceInfo&& surface_info,
            SwapchainParams&& params
        );

        ~Swapchain();

        Swapchain(Swapchain&& other) noexcept = default;

        Swapchain& operator=(Swapchain&& other) noexcept = default;

        void destroy(const VolkDeviceTable& fn_table, VkDevice device);

        /**
         * @brief Recreate the swapchain according to the new window specifications.
         *
         * @param physical_device The physical device handle.
         * @param fn_table The device function table.
         * @param device The device handle.
         * @param window The window from which the swapchain is created.
         * @return `SwapchainResult::Success` if the operation was successful, otherwise an error describing what went
         * wrong.
         */
        std::expected<bool, Error> recreate(
            VkPhysicalDevice physical_device,
            const VolkDeviceTable& fn_table,
            VkDevice device,
            GLFWwindow* window,
            bool out_of_date
        );

        void destroy_old(const VolkDeviceTable& fn_table, VkDevice device);

        [[nodiscard]] VkSwapchainKHR handle() const noexcept { return this->inner.handle; }

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
        // Hide default constructor, swapchains should be created using the factory function `Swapchain::create`.
        Swapchain() = default;

        InnerSwapchain inner; //!< The properties of a device's queue families.

        /**
         * @brief The surface of the window from which the swapchain is created.
         */
        ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE> surface;

        /**
         * @brief The surface format to use.
         */
        VkSurfaceFormatKHR surface_format = VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; //!< The presentation mode to use.

        VkExtent2D extent = VkExtent2D{.width = 0, .height = 0}; //!< The extent (dimensions) of the swapchain images.
        VkViewport viewport;
        VkRect2D scissor;
        VkClearValue clear_value = {0.5f, 0.5f, 0.5f, 1.0f};
        ExternalHandle<VkRenderPass, VK_NULL_HANDLE> render_pass;

        /**
         * @brief The parameters used to create a new swapchain.
         */
        struct CreationParams {
            u32 image_count = 0;
            VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
        } creation_params;

        /**
         * @brief Collection of semaphores used to sync access by the device to the swapchain images.
         *
         * Size matches the maximum number of frames in flight.
         */
        std::vector<ExternalHandle<VkSemaphore, VK_NULL_HANDLE>> image_semaphores;

        // Old inner swapchains must not be destroyed while their resources are still in use.
        /**
         * @brief A queue containing old inner swapchains, in order of deprecation.
         */
        std::queue<InnerSwapchain> old_swapchains;

        bool images_out_of_date = false;
    };
}
