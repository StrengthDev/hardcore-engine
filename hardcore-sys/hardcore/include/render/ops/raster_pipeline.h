#pragma once

#include "common.h"

#include "../../core/color.h"
#include "../../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

enum HCPrimitiveTopology {
    HCPrimitiveTopology_PointList,
    HCPrimitiveTopology_LineList,
    HCPrimitiveTopology_LineStrip,
    HCPrimitiveTopology_TriangleList,
    HCPrimitiveTopology_TriangleStrip,
    HCPrimitiveTopology_TriangleFan,

    // Geometry

    // HCPrimitiveTopology_LineListWithAdjacency,
    // HCPrimitiveTopology_LineStripWithAdjacency,
    // HCPrimitiveTopology_TriangleListWithAdjacency,
    // HCPrimitiveTopology_TriangleStripWithAdjacency,

    // Tesselation

    // HCPrimitiveTopology_PatchList,
};

enum HCPolygonMode {
    HCPolygonMode_Fill, //!< Polygon is fully drawn.
    HCPolygonMode_Line, //!< Polygon edges are drawn as line segments.
    HCPolygonMode_Point, //!< Polygon vertices are drawn as points.
};

enum HCTriangleCullMode {
    None,
    Back,
    Front,
    FrontAndBack,
};

enum HCTriangleFrontFace {
    HCTriangleFrontFace_Clockwise,
    HCTriangleFrontFace_CounterClockwise,
};

// TODO docs https://docs.vulkan.org/spec/latest/chapters/framebuffer.html#VkBlendFactor
enum HCBlendFactor {
    HCBlendFactor_Zero,
    HCBlendFactor_One,
    HCBlendFactor_SrcColor,
    HCBlendFactor_OneMinusSrcColor,
    HCBlendFactor_DstColor,
    HCBlendFactor_OneMinusDstColor,
    HCBlendFactor_SrcAlpha,
    HCBlendFactor_OneMinusSrcAlpha,
    HCBlendFactor_DstAlpha,
    HCBlendFactor_OneMinusDstAlpha,
    HCBlendFactor_ConstantColor,
    HCBlendFactor_OneMinusConstantColor,
    HCBlendFactor_ConstantAlpha,
    HCBlendFactor_OneMinusConstantAlpha,
    HCBlendFactor_SrcAlphaSaturate,
};

enum HCBlendOp {
    HCBlendOp_Add,
    HCBlendOp_Subtract,
    HCBlendOp_ReverseSubtract,
    HCBlendOp_Min,
    HCBlendOp_Max,
};

struct HCBlendMode {
    enum HCBlendFactor src_color_blend_factor;
    enum HCBlendFactor dst_color_blend_factor;
    enum HCBlendOp color_blend_op;
    enum HCBlendFactor src_alpha_blend_factor;
    enum HCBlendFactor dst_alpha_blend_factor;
    enum HCBlendOp alpha_blend_op;
};

struct HCRasterPipelineParams {
    enum HCPrimitiveTopology primitive_topology; //!< The primitive topology of the pipeline.
    bool discard_primitives; //!< Discard all primitives. Useful if your only interest are pipeline side effects.
    enum HCPolygonMode polygon_mode;
    enum HCTriangleCullMode triangle_cull_mode;
    enum HCTriangleFrontFace triangle_front_face;
    float line_width;
    bool depth_bias;
    float depth_bias_constant_factor;
    float depth_bias_clamp;
    float depth_bias_slope_factor;
    struct HCBlendMode blend_mode;
    struct HCColor blend_constants;
};

struct HCRasterPipeline {
    uint64_t id; //!< The ID of this raster pipeline within the device.
    uint32_t device; //!< The ID of the device which this raster pipeline belongs to.
};

struct HCResult hc_new_raster_pipeline(
    struct HCRasterPipeline* raster_pipeline,
    uint32_t device,
    HCOperationPredicate predicate,
    void* user_data
);

void hc_destroy_raster_pipeline(struct HCRasterPipeline* raster_pipeline);

#ifdef __cplusplus
}
#endif // __cplusplus
