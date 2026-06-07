
#pragma once

#include <render/ops/raster_pipeline.h>

#include <core/error.hpp>

#include <render/vulkan.hpp>

#include <expected>

namespace hc::render::pipeline {
    struct RasterPipelineInfo {
        // Rasterization
        VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        VkBool32 discard_primitives = VK_TRUE;
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_MAX_ENUM;
        VkCullModeFlags triangle_cull_mode = VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        VkFrontFace triangle_front_face = VK_FRONT_FACE_MAX_ENUM;
        float line_width = 0.0f;
        VkBool32 depth_bias = VK_FALSE;
        float depth_bias_constant_factor = 0.0f;
        float depth_bias_clamp = 0.0f;
        float depth_bias_slope_factor = 0.0f;

        // Blending
        VkPipelineColorBlendAttachmentState blend_mode = {};
        float blend_constants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        static std::expected<RasterPipelineInfo, Error> create(HCRasterPipelineInfo const& params);
    };
}
