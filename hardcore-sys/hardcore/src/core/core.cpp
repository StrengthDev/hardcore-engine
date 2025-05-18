#include <pch.hpp>

#include "log.hpp"

#include <core/core.h>
#ifndef HC_HEADLESS
#include <core/window.hpp>
#endif // HC_HEADLESS

#include <render/renderer.hpp>

int hc_init(HCInitParams params) {
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
    if (!hc::window::init_context()) {
        return -1;
    }
#endif // HC_HEADLESS

    if (hc::render::init(params.app, params.render_params) != hc::render::InstanceResult::Success) {
        return -1;
    }

    return 0;
}

int hc_term() {
    // TODO destroy all windows

    if (hc::render::term() != hc::render::InstanceResult::Success) {
        return -1;
    }

#ifndef HC_HEADLESS
    hc::window::terminate_context();
#endif // HC_HEADLESS

    return 0;
}
