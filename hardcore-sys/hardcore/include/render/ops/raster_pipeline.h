#pragma once

#include "common.h"

#include "../../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

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
