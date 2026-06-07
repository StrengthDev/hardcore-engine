
#include <pch.hpp>

#include "raster_pipeline.hpp"

#include <util/bits.hpp>

namespace hc::render::pipeline {
    std::expected<vk::PipelineLayout, Error> create_layout(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<std::reference_wrapper<Shader const>> const& shaders
    ) {
        std::vector<VkPushConstantRange> push_constant_ranges;
        push_constant_ranges.reserve(shaders.size());

        for (auto const& shader_ref : shaders) {
            auto const& shader = shader_ref.get();

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
        std::vector<std::reference_wrapper<Shader const>> const& shaders,
        HCRasterPipelineInfo const& info
    ) {
        RasterPipeline pipeline;

        for (auto const& shader_ref : shaders) {
            if (auto const& shader = shader_ref.get(); shader.stage_flag() == VK_SHADER_STAGE_FRAGMENT_BIT) {
                pipeline.color_attachment_count = static_cast<u32>(shader.outputs().size());
                break;
            }
        }

        if (!pipeline.color_attachment_count) {
            HC_ERROR("Raster pipelines must have at least 1 color attachment");
            return Error(HCError_InvalidParams);
        }

        auto info_result = RasterPipelineInfo::create(info);
        if (!info_result) {
            return info_result.error();
        }
        pipeline.info = *std::move(info_result);

        auto processor_result = PushConstantsProcessor::create(shaders.front().get().push_constants());
        if (!processor_result) {
            return processor_result.error();
        }
        pipeline.push_constants_processor = *std::move(processor_result);

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
            instance.extract_handle().destroy(fn_table, device);
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
                this->info,
                this->layout,
                this->color_attachment_count,
                render_pass,
                subpass
            );
            if (!result) {
                return result.error();
            }

            this->instance_keys.emplace(result.value().vk_handle(), std::pair(render_pass, subpass));
            this->instances.emplace(key, Instance { .instance = *std::move(result), .ref_count = 0 });
        }

        auto& [instance, ref_count] = this->instances[key];

        ref_count++;

        return instance.vk_handle();
    }

    std::optional<vk::GraphicsPipeline> RasterPipeline::free_instance(VkPipeline vk_handle) noexcept {
        auto [render_pass, subpass] = this->instance_keys[vk_handle];
        InstanceKey const key = { .render_pass = render_pass, .subpass = subpass };

        std::optional<vk::GraphicsPipeline> handle;

        if (!this->instances.contains(key)) {
            return handle;
        }

        auto& [instance, ref_count] = this->instances[key];

        ref_count--;

        if (!ref_count) {
            handle = instance.extract_handle();

            this->instances.erase(key);
            this->instance_keys.erase(vk_handle);
        }

        return handle;
    }

    std::expected<void, Error> RasterPipeline::fill_push_constants_buffer(
        std::vector<u8>& buffer,
        std::span<void const*> const& constant_ptrs
    ) const noexcept {
        return this->push_constants_processor(buffer, constant_ptrs);
    }

    Sz RasterPipeline::InstanceKeyHash::operator()(InstanceKey const& key) const noexcept {
        Sz const handle_hash = std::hash<VkRenderPass> {}(key.render_pass);
        Sz const subpass_hash = std::hash<u64> {}(reverse_bits(static_cast<u64>(key.subpass)));

        return handle_hash ^ subpass_hash;
    }
}
