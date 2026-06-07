
#pragma once

#include "common.h"
#include "raster_pipeline.h"
#include "render_pass.h"

#include "../../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief A persistent draw command.
 */
struct HCDraw {
    uint64_t id; //!< The ID of this draw command within the device.
    uint32_t device; //!< The ID of the device which this draw command belongs to.
};

struct HCResult hc_new_draw(
    struct HCDraw* draw,
    struct HCRenderPass const* render_pass,
    uint32_t subpass,
    struct HCRasterPipeline const* pipeline,
    uint32_t vertex_count,
    uint32_t instance_count
);

void hc_destroy_draw(struct HCDraw* draw);

struct HCResult hc_set_draw_push_constants(struct HCDraw const* draw, void const** push_constants, uint32_t count);

#ifdef __cplusplus
}
#endif // __cplusplus
