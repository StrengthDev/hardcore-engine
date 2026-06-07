
#include <pch.hpp>

#include <render/ops/raster_pipeline.h>

#include "../../validation.hpp"

#include <core/error.hpp>
#include <core/log.hpp>
#include <render/renderer.hpp>
#include <render/shader/shader.hpp>

#include <util/number.hpp>

#include <span>
#include <vector>

HCBlendMode const HC_NONE_BLEND_MODE = {
    HCBlendFactor_One,
    HCBlendFactor_Zero,
    HCBlendOp_Add,
    HCBlendFactor_One,
    HCBlendFactor_Zero,
    HCBlendOp_Add
};

HCBlendMode const HC_ALPHA_BLENDING_BLEND_MODE = {
    HCBlendFactor_SrcAlpha,
    HCBlendFactor_OneMinusSrcAlpha,
    HCBlendOp_Add,
    HCBlendFactor_One,
    HCBlendFactor_OneMinusSrcAlpha,
    HCBlendOp_Add
};

HCBlendMode const HC_PREMULTIPLIED_ALPHA_BLENDING_BLEND_MODE = {
    HCBlendFactor_One,
    HCBlendFactor_OneMinusSrcAlpha,
    HCBlendOp_Add,
    HCBlendFactor_One,
    HCBlendFactor_OneMinusSrcAlpha,
    HCBlendOp_Add
};

HCResult hc_new_raster_pipeline(
    HCRasterPipeline* raster_pipeline,
    u32 device,
    HCShader const* shaders,
    u32 shader_count,
    HCRasterPipelineInfo info
) {
    HC_VALIDATE_PTR_RE(raster_pipeline, "raster pipeline");
    HC_VALIDATE_PTR_RE(shaders, "shaders");
    HC_VALIDATE_RE(shader_count >= 2, "Raster pipeline must have at least 2 shaders");

    std::vector<std::reference_wrapper<hc::render::Shader const>> shader_vec;
    for (auto const& shader : std::span(shaders, shader_count)) {
        HC_VALIDATE_PTR_RE(shader.inner, "shader data");

        shader_vec.emplace_back(*static_cast<hc::render::Shader const*>(shader.inner));
    }

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto const result = device_result.value()->create_raster_pipeline(shader_vec, info);
    if (!result) {
        return result.error();
    }

    *raster_pipeline = {
        .id = *result,
        .device = device,
    };

    return { .success = true };
}

void hc_destroy_raster_pipeline(HCRasterPipeline* raster_pipeline) {
    HC_VALIDATE_PTR(raster_pipeline, "raster pipeline");

    auto device_result = hc::render::device_at(raster_pipeline->device);
    if (!device_result) {
        return;
    }

    device_result.value()->destroy_raster_pipeline(raster_pipeline->id);

    *raster_pipeline = {};
}
