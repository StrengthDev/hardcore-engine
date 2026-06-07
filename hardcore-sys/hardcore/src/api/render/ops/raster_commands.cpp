
#include <pch.hpp>

#include <render/ops/raster_commands.h>

#include "../../validation.hpp"

#include <render/renderer.hpp>

#include <core/error.hpp>
#include <core/log.hpp>
#include <util/number.hpp>

HCResult hc_new_draw(
    HCDraw* draw,
    HCRenderPass const* render_pass,
    u32 subpass,
    HCRasterPipeline const* pipeline,
    u32 vertex_count,
    u32 instance_count
) {
    HC_VALIDATE_PTR_RE(draw, "draw");
    HC_VALIDATE_PTR_RE(render_pass, "render pass");
    HC_VALIDATE_PTR_RE(pipeline, "raster pipeline");
    HC_VALIDATE_RE(render_pass->device == pipeline->device, "Render pass and pipeline devices do not match");

    auto device_result = hc::render::device_at(render_pass->device);
    if (!device_result) {
        return device_result.error();
    }

    auto const result = device_result.value()->create_draw(render_pass->id, subpass, pipeline->id, vertex_count, instance_count);
    if (!result) {
        return result.error();
    }

    *draw = {
        .id = *result,
        .device = render_pass->device,
    };

    return { .success = true };
}

void hc_destroy_draw(HCDraw* draw) {
    HC_VALIDATE_PTR(draw, "draw");

    auto device_result = hc::render::device_at(draw->device);
    if (!device_result) {
        return;
    }

    device_result.value()->destroy_draw(draw->id);

    *draw = {};
}

HCResult hc_set_draw_push_constants(HCDraw const* draw, void const** push_constants, u32 count) {
    HC_VALIDATE_PTR_RE(draw, "draw");
    HC_VALIDATE_RE(count > 0, "Invalid push constant count");
    HC_VALIDATE_PTR_RANGE_RE(push_constants, count, "push constants");

    auto device_result = hc::render::device_at(draw->device);
    if (!device_result) {
        return device_result.error();
    }

    auto result = device_result.value()->set_draw_push_constants(draw->id, std::span(push_constants, count));
    if (!result) {
        return result.error();
    }

    return { .success = true };
}
