
#pragma once

#include "raster_pipeline_params.hpp"
#include "shader_stages.hpp"

#include "../shader/shader.hpp"

#include <core/error.hpp>

#include <render/vulkan.hpp>

namespace hc::render::pipeline {
    class RasterPipelineInstance {
    public:
        [[nodiscard]]
        static std::expected<RasterPipelineInstance, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkPipelineCache cache,
            ShaderStages const& shaders,
            RasterPipelineParams const& params,
            VkPipelineLayout layout,
            u32 attachment_count,
            VkRenderPass render_pass,
            u32 subpass
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        [[nodiscard]] VkPipeline vk_handle() const noexcept;

    private:
        vk::GraphicsPipeline handle;
    };
}
