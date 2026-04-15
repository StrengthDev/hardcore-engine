
#pragma once

#include "memory/memory.hpp"
#include "render/resource/buffer.hpp"
#include "render/resource/texture.hpp"
#include "swapchain/swapchain.hpp"
#include "swapchain/swapchain_instance.hpp"

#include <core/glfw.hpp>

#include <util/number.hpp>

#include <variant>

namespace hc::render::device {
    class Cleaner {
    public:
        Cleaner();

        ~Cleaner();

        Cleaner(Cleaner&&) noexcept = default;
        Cleaner& operator=(Cleaner&&) noexcept = default;

        void clear(VolkDeviceTable const& fn_table, VkDevice device, memory::Memory& memory);

        void tick(VolkDeviceTable const& fn_table, VkDevice device, memory::Memory& memory);

        void yield_window(GLFWwindow* window, swapchain::Swapchain&& swapchain);
        void yield_swapchain(swapchain::SwapchainInstance&& instance);
        void yield_buffer(buffer::Buffer&& buffer);
        void yield_dynamic_buffer(buffer::DynamicBuffer&& buffer);
        void yield_texture(texture::Texture&& texture);

    private:
        struct Window {
            GLFWwindow* window;
            swapchain::Swapchain swapchain;
        };

        typedef std::variant<
            Window,
            swapchain::SwapchainInstance,
            buffer::Buffer,
            buffer::DynamicBuffer,
            texture::Texture
        > DestructionQueueItem;

        template<class... Ts>
        struct DestructionHandler : Ts... {
            using Ts::operator()...;
        };

        std::vector<std::vector<DestructionQueueItem>> cleanup_queues;
        std::vector<DestructionQueueItem> cleanup_submissions;

        u8 frame_mod = 0;
    };
}
