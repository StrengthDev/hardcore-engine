#pragma once

#include "scheduler.hpp"
#include "graph.hpp"
#include "destruction_mark.hpp"
#include "swapchain.hpp"

#include "memory/memory.hpp"

#include "../resource/descriptor.hpp"
#include "../resource/buffer.hpp"
#include "../resource/texture.hpp"

#include <core/glfw.hpp>

#include <render/buffer.h>

#include <optional>

namespace hc::render::device {
    enum class DeviceResult : u32 {
        Success = 0,
        VkFailure,
        SurfaceFailure,
        SwapchainFailure,
        AllocFailure,
        TextureFailure,
    };

    class Device {
    public:
        Device(const Device&) = delete;

        Device& operator=(const Device&) = delete;

        static std::optional<Device> create(VkPhysicalDevice physical_handle, const std::vector<const char*>& layers);

        ~Device();

        Device(Device&& other) noexcept = default;

        Device& operator=(Device&& other) noexcept = default;

        void tick(u8 frame_mod, u8 next_frame_mod);

        [[nodiscard]] const char* name() const noexcept;

        [[nodiscard]] DeviceResult create_swapchain(VkInstance instance, GLFWwindow* window);

        void destroy_swapchain(VkInstance instance, GLFWwindow* window);

        [[nodiscard]] Result<buffer::Params, DeviceResult> new_buffer(
            HCBufferKind kind,
            resource::Descriptor&& descriptor,
            u64 count,
            bool writable
        );

        [[nodiscard]] Result<buffer::Params, DeviceResult> new_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable
        );

        [[nodiscard]] Result<buffer::DynamicParams, DeviceResult> new_dynamic_buffer(
            HCBufferKind kind,
            resource::Descriptor&& descriptor,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        [[nodiscard]] Result<buffer::DynamicParams, DeviceResult> new_dynamic_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        void destroy_buffer(u64 id);

        [[nodiscard]] std::expected<texture::Params, DeviceResult> create_texture(VkImageCreateInfo const& image_info);

        void destroy_texture(u64 id);

    private:
        Device() = default;

        void cleanup(u8 frame_mod);
        void present(u8 frame_mod);

        ExternalHandle<VkPhysicalDevice, VK_NULL_HANDLE> physical_handle;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        Scheduler scheduler;
        Graph graph;
        memory::Memory memory;
        std::unordered_map<GLFWwindow*, Swapchain> swapchains;

        std::vector<std::vector<DestructionMark>> cleanup_queues;
        std::vector<DestructionMark> cleanup_submissions;

        ExternalHandle<VkDevice, VK_NULL_HANDLE> handle;
        VolkDeviceTable fn_table = {};
    };
}
