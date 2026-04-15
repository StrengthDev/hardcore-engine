#pragma once

#include "common.h"

#include "../resource/texture.h"

#include "../../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

struct HCSubpassInputAttachment {
    uint64_t texture_id;
    struct HCTextureViewParams texture_view_params;
    uint32_t index; //!< Corresponds to the `input_attachment_index` decorator in shader code.
    struct HCDependency dependency;
};

struct HCSubpassOutputAttachment {
    uint64_t texture_id;
    struct HCTextureViewParams texture_view_params;
    uint32_t location; //!< Corresponds to the `location` decorator in shader code.
    struct HCDependency dependency;
};

struct HCSubpassDepthStencilAttachment {
    uint64_t texture_id;
    struct HCTextureViewParams texture_view_params;
    struct HCDependency dependency;
};

struct HCSubpass {
    struct HCSubpassInputAttachment const* inputs;
    uint32_t input_count;
    struct HCSubpassOutputAttachment const* outputs;
    uint32_t output_count;
    struct HCSubpassDepthStencilAttachment const* depth_stencil_attachment;
};

struct HCRenderPass {
    uint64_t id; //!< The ID of this render pass within the device.
    uint32_t device; //!< The ID of the device which this render pass belongs to.
};

struct HCResult hc_new_render_pass(
    struct HCRenderPass* render_pass,
    uint32_t device,
    struct HCSubpass const* subpasses,
    uint32_t subpass_count,
    HCOperationPredicate predicate,
    void* user_data
);

void hc_destroy_render_pass(struct HCRenderPass* render_pass);

#ifdef __cplusplus
}
#endif // __cplusplus
