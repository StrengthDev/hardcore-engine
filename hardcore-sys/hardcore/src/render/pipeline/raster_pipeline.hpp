
#pragma once

#include "raster_pipeline_instance.hpp"
#include "raster_pipeline_params.hpp"
#include "shader_stages.hpp"

#include "../shader/shader.hpp"

#include <core/error.hpp>

#include <util/bank.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace hc::render::pipeline {
    class RasterPipeline {
    public:
        [[nodiscard]]
        static std::expected<RasterPipeline, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<Shader> const& shaders,
            HCRasterPipelineParams const& params
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        [[nodiscard]]
        std::expected<VkPipeline, Error> get_instance(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkPipelineCache cache,
            VkRenderPass render_pass,
            u32 subpass
        ) noexcept;

        void free_instance(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkRenderPass render_pass,
            u32 subpass
        ) noexcept;

    private:
        RasterPipeline() = default;

        struct InstanceKey {
            VkRenderPass render_pass;
            u32 subpass;

            bool operator==(InstanceKey const&) const noexcept = default;
        };

        struct InstanceKeyHash {
            Sz operator()(InstanceKey const& key) const noexcept;
        };

        struct Instance {
            RasterPipelineInstance instance;
            u32 ref_count;
        };

        ShaderStages shaders;
        RasterPipelineParams params;
        vk::PipelineLayout layout;
        u32 color_attachment_count = 0;

        std::unordered_map<InstanceKey, Instance, InstanceKeyHash> instances;
    };
}
