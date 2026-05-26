
#include <pch.hpp>

#include "raster_pipeline.hpp"

#include <util/bits.hpp>

namespace hc::render::pipeline {
    std::expected<vk::PipelineLayout, Error> create_layout(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<Shader> const& shaders
    ) {
        std::vector<VkPushConstantRange> push_constant_ranges;
        push_constant_ranges.reserve(shaders.size());

        for (auto const& shader : shaders) {
            if (auto const& push_constant = shader.push_constants(); push_constant) {
                push_constant_ranges.push_back(
                    VkPushConstantRange {
                        .stageFlags = static_cast<VkShaderStageFlags>(shader.stage_flag()),
                        .offset = push_constant->offset,
                        .size = static_cast<u32>(push_constant->descriptor.size()),
                    }
                );
            }
        }

        // TODO check for shader compatability issues

        VkPipelineLayoutCreateInfo const layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = static_cast<u32>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data(),
        };

        return vk::PipelineLayout::create(fn_table, device, layout_info);
    }

    std::expected<RasterPipeline, Error> RasterPipeline::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<Shader> const& shaders,
        HCRasterPipelineParams const& params
    ) {
        RasterPipeline pipeline;

        for (auto const& shader : shaders) {
            if (shader.stage_flag() == VK_SHADER_STAGE_FRAGMENT_BIT) {
                pipeline.color_attachment_count = static_cast<u32>(shader.outputs().size());
                break;
            }
        }

        if (!pipeline.color_attachment_count) {
            HC_ERROR("Raster pipelines must have at least 1 color attachment");
            return Error(HCError_InvalidParams);
        }

        auto params_result = RasterPipelineParams::create(params);
        if (!params_result) {
            return params_result.error();
        }
        pipeline.params = *std::move(params_result);

        auto layout_result = create_layout(fn_table, device, shaders);
        if (!layout_result) {
            return layout_result.error();
        }
        pipeline.layout = *std::move(layout_result);

        auto shaders_result = ShaderStages::create(fn_table, device, shaders);
        if (!shaders_result) {
            pipeline.destroy(fn_table, device);
            return shaders_result.error();
        }
        pipeline.shaders = *std::move(shaders_result);

        return pipeline;
    }

    void RasterPipeline::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (auto& [instance, _] : this->instances | std::views::values) {
            instance.destroy(fn_table, device);
        }
        this->instances.clear();

        this->layout.destroy(fn_table, device);
        this->shaders.destroy(fn_table, device);
    }

    std::expected<VkPipeline, Error> RasterPipeline::get_instance(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkPipelineCache cache,
        VkRenderPass render_pass,
        u32 subpass
    ) noexcept {
        InstanceKey const key = { .render_pass = render_pass, .subpass = subpass };

        if (!this->instances.contains(key)) {
            auto result = RasterPipelineInstance::create(
                fn_table,
                device,
                cache,
                this->shaders,
                this->params,
                this->layout,
                this->color_attachment_count,
                render_pass,
                subpass
            );
            if (!result) {
                return result.error();
            }

            this->instances.emplace(key, Instance { .instance = *std::move(result), .ref_count = 0 });
        }

        auto& [instance, ref_count] = this->instances[key];

        ref_count++;

        return instance.vk_handle();
    }

    void RasterPipeline::free_instance(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkRenderPass render_pass,
        u32 subpass
    ) noexcept {
        InstanceKey const key = { .render_pass = render_pass, .subpass = subpass };

        if (!this->instances.contains(key)) {
            return;
        }

        auto& [instance, ref_count] = this->instances[key];

        ref_count--;

        if (!ref_count) {
            instance.destroy(fn_table, device);

            this->instances.erase(key);
        }
    }

    Sz RasterPipeline::InstanceKeyHash::operator()(InstanceKey const& key) const noexcept {
        Sz const handle_hash = std::hash<VkRenderPass> {}(key.render_pass);
        Sz const subpass_hash = std::hash<u64> {}(reverse_bits(static_cast<u64>(key.subpass)));

        return handle_hash ^ subpass_hash;
    }
}
