
#include <pch.hpp>

#include "raster_pipeline_info.hpp"

#include <core/color.hpp>

#include <util/static_map.hpp>

namespace hc::render::pipeline {
    static StaticMap<BasicKey<HCPrimitiveTopology, HCPrimitiveTopology_TriangleFan>, VkPrimitiveTopology> constexpr PRIMITIVE_TOPOLOGY_MAP = {
        { HCPrimitiveTopology_PointList, VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
        { HCPrimitiveTopology_LineList, VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
        { HCPrimitiveTopology_LineStrip, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP },
        { HCPrimitiveTopology_TriangleList, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
        { HCPrimitiveTopology_TriangleStrip, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP },
        { HCPrimitiveTopology_TriangleFan, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN },
    };

    static StaticMap<BasicKey<HCPolygonMode, HCPolygonMode_Point>, VkPolygonMode> constexpr POLYGON_MODE_MAP = {
        { HCPolygonMode_Fill, VK_POLYGON_MODE_FILL },
        { HCPolygonMode_Line, VK_POLYGON_MODE_LINE },
        { HCPolygonMode_Point, VK_POLYGON_MODE_POINT },
    };

    static StaticMap<BasicKey<HCTriangleCullMode, FrontAndBack>, VkCullModeFlags> constexpr CULL_MODE_MAP = {
        { None, VK_CULL_MODE_NONE },
        { Back, VK_CULL_MODE_BACK_BIT },
        { Front, VK_CULL_MODE_FRONT_BIT },
        { FrontAndBack, VK_CULL_MODE_FRONT_AND_BACK },
    };

    static StaticMap<BasicKey<HCTriangleFrontFace, HCTriangleFrontFace_CounterClockwise>, VkFrontFace> constexpr FRONT_FACE_MAP = {
        { HCTriangleFrontFace_Clockwise, VK_FRONT_FACE_CLOCKWISE },
        { HCTriangleFrontFace_CounterClockwise, VK_FRONT_FACE_COUNTER_CLOCKWISE },
    };

    static StaticMap<BasicKey<HCBlendFactor, HCBlendFactor_SrcAlphaSaturate>, VkBlendFactor> constexpr BLEND_FACTOR_MAP = {
        { HCBlendFactor_Zero, VK_BLEND_FACTOR_ZERO },
        { HCBlendFactor_One, VK_BLEND_FACTOR_ONE },
        { HCBlendFactor_SrcColor, VK_BLEND_FACTOR_SRC_COLOR },
        { HCBlendFactor_OneMinusSrcColor, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR },
        { HCBlendFactor_DstColor, VK_BLEND_FACTOR_DST_COLOR },
        { HCBlendFactor_OneMinusDstColor, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR },
        { HCBlendFactor_SrcAlpha, VK_BLEND_FACTOR_SRC_ALPHA },
        { HCBlendFactor_OneMinusSrcAlpha, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
        { HCBlendFactor_DstAlpha, VK_BLEND_FACTOR_DST_ALPHA },
        { HCBlendFactor_OneMinusDstAlpha, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA },
        { HCBlendFactor_ConstantColor, VK_BLEND_FACTOR_CONSTANT_COLOR },
        { HCBlendFactor_OneMinusConstantColor, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR },
        { HCBlendFactor_ConstantAlpha, VK_BLEND_FACTOR_CONSTANT_ALPHA },
        { HCBlendFactor_OneMinusConstantAlpha, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA },
        { HCBlendFactor_SrcAlphaSaturate, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE },
    };

    static StaticMap<BasicKey<HCBlendOp, HCBlendOp_Max>, VkBlendOp> constexpr BLEND_OP_MAP = {
        { HCBlendOp_Add, VK_BLEND_OP_ADD },
        { HCBlendOp_Subtract, VK_BLEND_OP_SUBTRACT },
        { HCBlendOp_ReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT },
        { HCBlendOp_Min, VK_BLEND_OP_MIN },
        { HCBlendOp_Max, VK_BLEND_OP_MAX },
    };

    std::expected<RasterPipelineInfo, Error> RasterPipelineInfo::create(HCRasterPipelineInfo const& params) {
        auto const topology = PRIMITIVE_TOPOLOGY_MAP[params.primitive_topology];
        if (!topology) {
            HC_ERROR("Unknown primitive topology value");
            return Error(HCError_InvalidParams);
        }

        auto const polygon_mode = POLYGON_MODE_MAP[params.polygon_mode];
        if (!polygon_mode) {
            HC_ERROR("Unknown polygon mode value");
            return Error(HCError_InvalidParams);
        }

        auto const cull_mode = CULL_MODE_MAP[params.triangle_cull_mode];
        if (!cull_mode) {
            HC_ERROR("Unknown triangle mull mode value");
            return Error(HCError_InvalidParams);
        }

        auto const front_face = FRONT_FACE_MAP[params.triangle_front_face];
        if (!front_face) {
            HC_ERROR("Unknown triangle front face value");
            return Error(HCError_InvalidParams);
        }

        auto const src_color_blend_factor = BLEND_FACTOR_MAP[params.blend_mode.src_color_blend_factor];
        auto const dst_color_blend_factor = BLEND_FACTOR_MAP[params.blend_mode.dst_color_blend_factor];
        auto const src_alpha_blend_factor = BLEND_FACTOR_MAP[params.blend_mode.src_alpha_blend_factor];
        auto const dst_alpha_blend_factor = BLEND_FACTOR_MAP[params.blend_mode.dst_alpha_blend_factor];

        if (!src_color_blend_factor || !dst_color_blend_factor || !src_alpha_blend_factor || !dst_alpha_blend_factor) {
            HC_ERROR("Unknown blend factor value");
            return Error(HCError_InvalidParams);
        }

        auto const color_blend_op = BLEND_OP_MAP[params.blend_mode.color_blend_op];
        auto const alpha_blend_op = BLEND_OP_MAP[params.blend_mode.alpha_blend_op];

        if (!color_blend_op || !alpha_blend_op) {
            HC_ERROR("Unknown blend operation value");
            return Error(HCError_InvalidParams);
        }

        auto const blend_constants = clamp_color(params.blend_constants);

        return RasterPipelineInfo {
            .primitive_topology = *topology,
            .discard_primitives = params.discard_primitives,
            .polygon_mode = *polygon_mode,
            .triangle_cull_mode = *cull_mode,
            .triangle_front_face = *front_face,
            .line_width = params.line_width,
            .depth_bias = params.depth_bias ? VK_TRUE : VK_FALSE,
            .depth_bias_constant_factor = params.depth_bias_constant_factor,
            .depth_bias_clamp = params.depth_bias_clamp,
            .depth_bias_slope_factor = params.depth_bias_slope_factor,
            .blend_mode = {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = *src_color_blend_factor,
                .dstColorBlendFactor = *dst_color_blend_factor,
                .colorBlendOp = *color_blend_op,
                .srcAlphaBlendFactor = *src_alpha_blend_factor,
                .dstAlphaBlendFactor = *dst_alpha_blend_factor,
                .alphaBlendOp = *alpha_blend_op,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT,
            },
            .blend_constants = { blend_constants.r, blend_constants.g, blend_constants.b, blend_constants.a },
        };
    }
}
