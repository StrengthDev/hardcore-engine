
#pragma once

#include "push_constants_processor.hpp"
#include "raster_pipeline_instance.hpp"
#include "raster_pipeline_info.hpp"
#include "shader_stages.hpp"

#include "../shader/shader.hpp"

#include <core/error.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace hc::render::pipeline {
    class RasterPipeline {
    public:
        [[nodiscard]]
        static std::expected<RasterPipeline, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<std::reference_wrapper<Shader const>> const& shaders,
            HCRasterPipelineInfo const& info
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

        [[nodiscard]] std::optional<vk::GraphicsPipeline> free_instance(VkPipeline vk_handle) noexcept;

        [[nodiscard]] std::expected<void, Error> fill_push_constants_buffer(
            std::vector<u8>& buffer,
            std::span<void const*> const& constant_ptrs
        ) const noexcept;

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
        RasterPipelineInfo info;
        vk::PipelineLayout layout;
        PushConstantsProcessor push_constants_processor;

        u32 color_attachment_count = 0;

        std::unordered_map<InstanceKey, Instance, InstanceKeyHash> instances;
        std::unordered_map<VkPipeline, std::pair<VkRenderPass, u32>> instance_keys;
    };
}
