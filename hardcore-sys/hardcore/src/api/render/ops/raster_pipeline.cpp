
#include <pch.hpp>

#include <render/ops/raster_pipeline.h>

#include <core/log.hpp>
#include <render/renderer.hpp>

HCResult hc_new_raster_pipeline(
    HCRasterPipeline* raster_pipeline,
    uint32_t device,
    HCOperationPredicate,
    //predicate,
    void* //user_data


) {
    if (!raster_pipeline) {
        HC_ERROR("Null rester pipeline pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    // TODO

    return {.success = true};
}

void hc_destroy_raster_pipeline(HCRasterPipeline* raster_pipeline) {
    if (!raster_pipeline) {
        HC_WARN("Null rester pipeline pointer");
        return;
    }
    const auto device_id = raster_pipeline->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    // TODO

    *raster_pipeline = {};
}
