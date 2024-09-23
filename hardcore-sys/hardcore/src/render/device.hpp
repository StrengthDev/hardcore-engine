#pragma once

#include <optional>

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <render/buffer.h>

#include "device/scheduler.hpp"
#include "device/graph.hpp"
#include "device/memory.hpp"
#include "device/destruction_mark.hpp"
#include "device/swapchain.hpp"
#include "resource/descriptor.hpp"
#include "resource/buffer.hpp"

namespace hc::render {
    enum class DeviceResult : u32 {
        Success = 0,
        VkFailure,
        SurfaceFailure,
        SwapchainFailure,
        AllocFailure,
    };

    class Device {
    public:
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        static std::optional<Device> create(VkPhysicalDevice physical_handle, const std::vector<const char *> &layers);

        ~Device();

        Device(Device &&other) noexcept = default;

        Device &operator=(Device &&other) noexcept = default;

        void tick(u8 frame_mod, u8 next_frame_mod);

        [[nodiscard]] const char *name() const noexcept;

        [[nodiscard]] DeviceResult create_swapchain(VkInstance instance, GLFWwindow *window);

        void destroy_swapchain(VkInstance instance, GLFWwindow *window, u8 frame_mod);

        [[nodiscard]] Result<buffer::Params, DeviceResult>
        new_buffer(HCBufferKind kind, resource::Descriptor &&descriptor,
                   u64 count, bool writable);

        [[nodiscard]] Result<buffer::Params, DeviceResult>
        new_index_buffer(HCPrimitive index_type, u64 count, bool writable);

        [[nodiscard]] Result<buffer::DynamicParams, DeviceResult>
        new_dynamic_buffer(HCBufferKind kind, resource::Descriptor &&descriptor, u64 count, bool writable,
                           u8 frame_mod);

        [[nodiscard]] Result<buffer::DynamicParams, DeviceResult>
        new_dynamic_index_buffer(HCPrimitive index_type, u64 count, bool writable, u8 frame_mod);

        void destroy_buffer(u64 id, u8 frame_mod);

    private:
        Device() = default;

        ExternalHandle<VkPhysicalDevice, VK_NULL_HANDLE> physical_handle;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        device::Scheduler scheduler;
        device::Graph graph;
        device::Memory memory;
        std::unordered_map<GLFWwindow *, device::Swapchain> swapchains;

        std::vector<std::vector<device::DestructionMark>> cleanup_queues;

        ExternalHandle<VkDevice, VK_NULL_HANDLE> handle;
        VolkDeviceTable fn_table = {};

    };
}
