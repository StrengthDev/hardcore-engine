
#include <pch.hpp>

#include "cleaner.hpp"

#include "render/vars.hpp"

#include <window/context.hpp>

#include <util/flow.hpp>
#include <util/variant_visitor.hpp>

namespace hc::render::device {
    Cleaner::Cleaner() : cleanup_queues(max_frames_in_flight()) {}

    Cleaner::~Cleaner() {
        HC_ASSERT(this->cleanup_submissions.empty(), "There must be no items left for cleanup before destruction");

        for (auto const& queue : this->cleanup_queues) {
            HC_ASSERT(queue.empty(), "There must be no items left for cleanup before destruction");
        }
    }

    void Cleaner::clear(VolkDeviceTable const& fn_table, VkDevice device, memory::Memory& memory) {
        for (u8 i = 0; i < max_frames_in_flight() + 1; ++i) {
            tick(fn_table, device, memory);
        }
    }

    void Cleaner::tick(VolkDeviceTable const& fn_table, VkDevice device, memory::Memory& memory) {
        auto& cleanup_queue = this->cleanup_queues[this->frame_mod];
        for (auto& item : cleanup_queue) {
            std::visit(
                VariantVisitor {
                    [&fn_table, &device](Window& window) {
                        window.swapchain.destroy(fn_table, device);
                        window::Context::instance().destroy_window(window.window);
                    },
                    [&fn_table, &device](swapchain::SwapchainInstance& swapchain) {
                        swapchain.destroy(fn_table, device);
                    },
                    [&memory](buffer::Buffer& buffer) {
                        memory.free_buffer(buffer.memory_ref());
                    },
                    [&memory](buffer::DynamicBuffer& buffer) {
                        memory.free_dynamic_buffer(buffer.memory_ref());
                    },
                    [&fn_table, &device, &memory](texture::Texture& texture) {
                        texture.destroy(fn_table, device);
                        memory.free_texture(texture.memory_ref());
                    },
                    [&fn_table, &device](vk::RenderPass& render_pass) {
                        render_pass.destroy(fn_table, device);
                    },
                    [&fn_table, &device](pipeline::RasterPipeline& pipeline) {
                        pipeline.destroy(fn_table, device);
                    },
                    [&fn_table, &device](vk::GraphicsPipeline& pipeline) {
                        pipeline.destroy(fn_table, device);
                    },
                },
                item
            );
        }
        cleanup_queue.clear();
        std::swap(cleanup_queue, this->cleanup_submissions);

        this->frame_mod = (this->frame_mod + 1) % max_frames_in_flight();
    }

    void Cleaner::yield_window(GLFWwindow* window, swapchain::Swapchain&& swapchain) {
        this->cleanup_submissions.emplace_back(Window { .window = window, .swapchain = std::move(swapchain) });
    }

    void Cleaner::yield_swapchain(swapchain::SwapchainInstance&& instance) {
        this->cleanup_submissions.emplace_back(std::move(instance));
    }

    void Cleaner::yield_buffer(buffer::Buffer&& buffer) {
        this->cleanup_submissions.emplace_back(std::move(buffer));
    }

    void Cleaner::yield_dynamic_buffer(buffer::DynamicBuffer&& buffer) {
        this->cleanup_submissions.emplace_back(std::move(buffer));
    }

    void Cleaner::yield_texture(texture::Texture&& texture) {
        this->cleanup_submissions.emplace_back(std::move(texture));
    }

    void Cleaner::yield_render_pass(vk::RenderPass&& render_pass) {
        this->cleanup_submissions.emplace_back(std::move(render_pass));
    }

    void Cleaner::yield_raster_pipeline(pipeline::RasterPipeline&& pipeline) {
        this->cleanup_submissions.emplace_back(std::move(pipeline));
    }

    void Cleaner::yield_graphics_pipeline_instance(vk::GraphicsPipeline&& pipeline) {
        this->cleanup_submissions.emplace_back(std::move(pipeline));
    }
}
