#pragma once

#include "cleaner.hpp"
#include "graph.hpp"
#include "scheduler.hpp"

#include "memory/memory.hpp"
#include "swapchain/swapchain.hpp"

#include "../vulkan.hpp"

#include "../resource/descriptor.hpp"
#include "../resource/buffer.hpp"
#include "../resource/texture.hpp"

#include <core/error.hpp>
#include <core/glfw.hpp>

#include <render/ops/render_pass.h>
#include <render/resource/buffer.h>

#include <util/concurrent_queue.hpp>
#include <util/number.hpp>
#include <util/user_predicate.hpp>

#include <span>

namespace hc::render::device {
    class Device {
    public:
        Device(const Device&) = delete;

        Device& operator=(const Device&) = delete;

        static std::expected<Device, Error> create(VkPhysicalDevice physical_handle, const std::vector<const char*>& layers);

        ~Device();

        Device(Device&&) noexcept = default;

        Device& operator=(Device&&) noexcept = default;

        std::expected<void, Error> tick(u8 frame_mod, u8 next_frame_mod);

        void finish();

        [[nodiscard]] const char* name() const noexcept;

        // Called from Window::create which may be running in another thread, however window creation is a special case,
        // even if Window::create is indeed being executed on another thread, the render thread should block on this
        // operation as the application code may perform some other operation after window creation which alters the
        // swapchain. So NO synchronization is done here.
        [[nodiscard]] std::expected<void, Error> create_swapchain(
            GLFWwindow* window,
            VkExtent2D extent
        );

        // Same as create_swapchain, this is called from Window::disable which may be running on another thread, and
        // just like creation, this is a special case as well. NO synchronization is done here.
        void destroy_swapchain(GLFWwindow* window);

        // Called from a window callback, which may be running in another thread, should be synchronized
        void resize_framebuffer(GLFWwindow const* window, VkExtent2D extent) noexcept;

        [[nodiscard]] std::expected<buffer::BufferData, Error> new_buffer(
            HCBufferKind kind,
            Descriptor&& descriptor,
            u64 count,
            bool writable
        );

        [[nodiscard]] std::expected<buffer::BufferData, Error> new_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable
        );

        [[nodiscard]] std::expected<buffer::DynamicBufferData, Error> new_dynamic_buffer(
            HCBufferKind kind,
            Descriptor&& descriptor,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        [[nodiscard]] std::expected<buffer::DynamicBufferData, Error> new_dynamic_index_buffer(
            HCPrimitive index_type,
            u64 count,
            bool writable,
            u8 frame_mod
        );

        void destroy_buffer(u64 id);
        void destroy_dynamic_buffer(u64 id);

        [[nodiscard]] std::expected<texture::TextureData, Error> create_texture(VkImageCreateInfo const& image_info);

        void destroy_texture(u64 id);

        std::expected<u64, Error> create_render_pass(
            std::span<HCSubpass const> const& subpasses,
            UserPredicate<Sz>&& predicate
        );

        void destroy_render_pass(u64 id);

    private:
        Device() = default;

        void update_framebuffers() noexcept;

        [[nodiscard]] std::expected<void, Error> present(u8 frame_mod);
        [[nodiscard]] std::expected<void, Error> present_queue_windows(
            u8 frame_mod,
            Queue const& queue,
            const std::vector<GLFWwindow const*>& windows
        );

        VkPhysicalDevice physical_handle = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        Scheduler scheduler;
        Graph graph;
        memory::Memory memory;

        struct FramebufferResize {
            GLFWwindow const* window;
            VkExtent2D extent;
        };

        std::unordered_map<u32, std::vector<GLFWwindow const*>> queue_windows;
        std::unordered_map<GLFWwindow const*, swapchain::Swapchain> swapchains;
        ConcurrentQueue<FramebufferResize> framebuffer_resizes;

        vk::PipelineCache pipeline_cache;

        Cleaner cleaner;

        vk::Device handle;
        VolkDeviceTable fn_table = {};
    };
}
