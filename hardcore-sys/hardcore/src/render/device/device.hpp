#pragma once

#include "scheduler.hpp"
#include "graph.hpp"
#include "destruction_mark.hpp"
#include "swapchain.hpp"

#include "memory/memory.hpp"

#include "../resource/descriptor.hpp"
#include "../resource/buffer.hpp"
#include "../resource/texture.hpp"

#include <core/error.hpp>
#include <core/glfw.hpp>

#include <render/buffer.h>

namespace hc::render::device {
    class Device {
    public:
        Device(const Device&) = delete;

        Device& operator=(const Device&) = delete;

        static std::expected<Device, Error> create(VkPhysicalDevice physical_handle, const std::vector<const char*>& layers);

        ~Device();

        Device(Device&& other) noexcept = default;

        Device& operator=(Device&& other) noexcept = default;

        std::expected<void, Error> tick(u8 frame_mod, u8 next_frame_mod);

        void finish(std::vector<u8> const& frame_mods);

        [[nodiscard]] const char* name() const noexcept;

        [[nodiscard]] std::expected<void, Error> create_swapchain(
            GLFWwindow* window,
            ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE>&& surface,
            VkExtent2D extent
        );

        void destroy_swapchain(GLFWwindow* window);

        [[nodiscard]] std::expected<buffer::Params, Error> new_buffer(
            HCBufferKind kind,
            Descriptor&& descriptor,
            u64 count,
            bool writable
        );

        [[nodiscard]] std::expected<buffer::Params, Error> new_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable
        );

        [[nodiscard]] std::expected<buffer::DynamicParams, Error> new_dynamic_buffer(
            HCBufferKind kind,
            Descriptor&& descriptor,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        [[nodiscard]] std::expected<buffer::DynamicParams, Error> new_dynamic_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        void destroy_buffer(u64 id);

        [[nodiscard]] std::expected<texture::Params, Error> create_texture(VkImageCreateInfo const& image_info);

        void destroy_texture(u64 id);

    private:
        Device() = default;

        void cleanup(u8 frame_mod);
        [[nodiscard]] std::expected<void, Error> present(u8 frame_mod);
        [[nodiscard]] std::expected<void, Error> present_queue_windows(
            u8 frame_mod,
            Queue const& queue,
            const std::vector<GLFWwindow*>& windows
        );

        ExternalHandle<VkPhysicalDevice, VK_NULL_HANDLE> physical_handle;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        Scheduler scheduler;
        Graph graph;
        memory::Memory memory;
        std::unordered_map<u32, std::vector<GLFWwindow*>> queue_windows;
        std::unordered_map<GLFWwindow*, Swapchain> swapchains;

        std::vector<std::vector<DestructionMark>> cleanup_queues;
        std::vector<DestructionMark> cleanup_submissions;

        ExternalHandle<VkDevice, VK_NULL_HANDLE> handle;
        VolkDeviceTable fn_table = {};
    };
}
