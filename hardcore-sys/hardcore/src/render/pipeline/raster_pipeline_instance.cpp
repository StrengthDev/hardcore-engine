
#include <pch.hpp>

#include "raster_pipeline_instance.hpp"

namespace hc::render::pipeline {
    static VkPipelineViewportStateCreateInfo constexpr VIEWPORT = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 0,
        .pViewports = nullptr,
        .scissorCount = 0,
        .pScissors = nullptr,
    };

    static VkDynamicState constexpr DYNAMIC_STATES[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    static VkPipelineDynamicStateCreateInfo constexpr DYNAMIC_STATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = sizeof(DYNAMIC_STATES) / sizeof(VkDynamicState),
        .pDynamicStates = DYNAMIC_STATES,
    };

    std::expected<RasterPipelineInstance, Error> RasterPipelineInstance::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkPipelineCache cache,
        ShaderStages const& shaders,
        RasterPipelineParams const& params,
        VkPipelineLayout layout,
        u32 attachment_count,
        VkRenderPass render_pass,
        u32 subpass
    ) {
        RasterPipelineInstance pipeline;

        // VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        //     .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .vertexBindingDescriptionCount = 0,
        //     .pVertexBindingDescriptions = nullptr,
        //     .vertexAttributeDescriptionCount = 0,
        //     .pVertexAttributeDescriptions = nullptr,
        // };

        VkPipelineInputAssemblyStateCreateInfo const input_assembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = params.primitive_topology,
            // Restart works by having an index with a special value in indexed draws, but we don't do indexed draws
            // because we force users to do vertex pulling
            .primitiveRestartEnable = VK_FALSE,
        };

        VkPipelineRasterizationStateCreateInfo const rasterization_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            // requires feature
            .rasterizerDiscardEnable = params.discard_primitives,
            .polygonMode = params.polygon_mode,
            .cullMode = params.triangle_cull_mode,
            .frontFace = params.triangle_front_face,
            .depthBiasEnable = params.depth_bias,
            .depthBiasConstantFactor = params.depth_bias_constant_factor,
            .depthBiasClamp = params.depth_bias_clamp,
            .depthBiasSlopeFactor = params.depth_bias_slope_factor,
            .lineWidth = params.line_width,
        };

        VkPipelineMultisampleStateCreateInfo const multisample_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        // VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .depthTestEnable = ,
        //     .depthWriteEnable = ,
        //     .depthCompareOp = ,
        //     .depthBoundsTestEnable = ,
        //     .stencilTestEnable = ,
        //     .front = ,
        //     .back = ,
        //     .minDepthBounds = ,
        //     .maxDepthBounds =
        // };

        std::vector blend_attachments(attachment_count, params.blend_mode);

        VkPipelineColorBlendStateCreateInfo const blend_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_NO_OP,
            .attachmentCount = attachment_count,
            .pAttachments = blend_attachments.data(),
            .blendConstants = {
                params.blend_constants[0],
                params.blend_constants[1],
                params.blend_constants[2],
                params.blend_constants[3],
            },
        };

        VkGraphicsPipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = shaders.stage_count(),
            .pStages = shaders.stages(),
            .pVertexInputState = nullptr,
            .pInputAssemblyState = &input_assembly,
            .pTessellationState = nullptr,
            .pViewportState = &VIEWPORT,
            .pRasterizationState = &rasterization_state,
            .pMultisampleState = &multisample_state,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &blend_state,
            .pDynamicState = &DYNAMIC_STATE_INFO,
            .layout = layout,
            .renderPass = render_pass,
            .subpass = subpass,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        auto result = vk::GraphicsPipeline::create(fn_table, device, cache, pipeline_info);
        if (!result) {
            pipeline.destroy(fn_table, device);
            return result.error();
        }
        pipeline.handle = *std::move(result);

        return pipeline;
    }

    void RasterPipelineInstance::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        this->handle.destroy(fn_table, device);
    }

    VkPipeline RasterPipelineInstance::vk_handle() const noexcept {
        return this->handle;
    }
}
