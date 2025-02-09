#include <pch.hpp>

#include "log.hpp"

#include <core/core.h>
#ifndef HC_HEADLESS
#include <core/glfw.hpp>
#endif // HC_HEADLESS

#include <render/renderer.hpp>

#ifndef HC_HEADLESS

void glfw_error_callback(int error_code, const char* description) {
    HC_ERROR("GLFW: " << description << "(code " << error_code << ')');
}

#endif // HC_HEADLESS

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
    if (!glfwInit()) {
        const char* description;
        glfwGetError(&description);
        HC_ERROR("Error initialising GLFW's context: %s\n" << description);
        return -1;
    }

    glfwSetErrorCallback(glfw_error_callback);
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
    glfwTerminate();
#endif // HC_HEADLESS

    return 0;
}
