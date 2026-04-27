
#include <pch.hpp>

#include <core/core.h>

#include <core/log.hpp>

#ifndef HC_HEADLESS
#include <window/context.hpp>
#endif // HC_HEADLESS

#include <render/renderer.hpp>

HCResult hc_init(HCInitParams params) {
#ifdef HC_LOGGING
    if (params.log_fn) {
        hc::set_log(params.log_fn);
    }
    if (params.start_span_fn && params.end_span_fn) {
        hc::set_span(params.start_span_fn, params.end_span_fn);
    }
    HC_INFO("hardcore-sys v" << HC_MAJOR << '.' << HC_MINOR << '.' << HC_PATCH);
#endif // HC_LOGGING

#ifndef HC_HEADLESS
    auto window_result = hc::window::Context::instance().init();
    if (!window_result) {
        return window_result.error();
    }
#endif // HC_HEADLESS

    auto renderer_result = hc::render::init(params.app, params.render_params);
    if (!renderer_result) {
        return renderer_result.error();
    }

    return {.success = true};
}

void hc_term() {
    hc::render::term();

#ifndef HC_HEADLESS
    hc::window::Context::instance().terminate();
#endif // HC_HEADLESS
}
