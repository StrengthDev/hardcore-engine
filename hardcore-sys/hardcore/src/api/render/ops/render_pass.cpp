
#include <pch.hpp>

#include <render/ops/render_pass.h>

#include "../../validation.hpp"

#include <core/error.hpp>
#include <core/log.hpp>
#include <render/renderer.hpp>

#include <util/number.hpp>
#include <util/user_predicate.hpp>

HCResult hc_new_render_pass(
    HCRenderPass* render_pass,
    u32 device,
    HCSubpass const* subpasses,
    uint32_t subpass_count,
    HCOperationPredicate predicate,
    void* user_data

) {
    HC_VALIDATE_PTR_RE(render_pass, "render pass");
    HC_VALIDATE_PTR_RE(subpasses, "subpasses");
    HC_VALIDATE_RE(subpass_count > 0, "There must be at least 1 subpass");

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto subpass_span = std::span(subpasses, subpass_count);
    auto predicate_fn = UserPredicate<Sz>(predicate, user_data);

    auto render_pass_result = (*device_result)->create_render_pass(subpass_span, std::move(predicate_fn));
    if (!render_pass_result) {
        return render_pass_result.error();
    }

    *render_pass = {
        .id = *render_pass_result,
        .device = device,
    };

    return { .success = true };
}

void hc_destroy_render_pass(HCRenderPass* render_pass) {
    HC_VALIDATE_PTR(render_pass, "render pass");

    const auto device_id = render_pass->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    (*device_result)->destroy_render_pass(render_pass->id);

    *render_pass = {};
}
