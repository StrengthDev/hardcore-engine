#pragma once

#include <optional>

#include <volk.h>

#include "device/scheduler.hpp"

namespace hc::render {
    class Device {
    public:
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        static std::optional<Device> create(VkPhysicalDevice physical_handle, const std::vector<const char *> &layers);

        ~Device();

        Device(Device &&other) noexcept;

        Device &operator=(Device &&other) noexcept;

    private:
        Device() = default;

        void destroy();

        VkPhysicalDevice physical_handle = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        device::Scheduler scheduler;
        VolkDeviceTable fn_table = {};
        VkDevice handle = VK_NULL_HANDLE;

    };
}
