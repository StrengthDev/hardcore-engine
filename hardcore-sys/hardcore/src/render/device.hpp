#pragma once

#include <optional>

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include "device/scheduler.hpp"
#include "device/destruction_mark.hpp"
#include "device/swapchain.hpp"

namespace hc::render {
    enum class DeviceResult : u32 {
        Success = 0,
        VkFailure,
        SurfaceFailure,
        SwapchainFailure,
    };

    class Device {
    public:
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        static std::optional<Device> create(VkPhysicalDevice physical_handle, const std::vector<const char *> &layers);

        ~Device();

        Device(Device &&other) noexcept;

        Device &operator=(Device &&other) noexcept;

        void tick(u8 frame_mod);

        [[nodiscard]] const char *name() const noexcept;

        [[nodiscard]] DeviceResult create_swapchain(VkInstance instance, GLFWwindow *window);

        void destroy_swapchain(VkInstance instance, GLFWwindow *window, u8 frame_mod);

    private:
        Device() = default;

        void destroy();

        VkPhysicalDevice physical_handle = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        device::Scheduler scheduler;
        std::unordered_map<GLFWwindow *, Swapchain> swapchains;

        std::vector<std::vector<device::DestructionMark>> cleanup_queues;

        VkDevice handle = VK_NULL_HANDLE;
        VolkDeviceTable fn_table = {};

    };
}
