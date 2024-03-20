#include <pch.hpp>

#ifndef HC_HEADLESS
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#endif // HC_HEADLESS

#include <core/core.h>
#include "log.hpp"

#ifndef HC_HEADLESS

void glfw_error_callback(int error_code, const char *description) {
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
#endif // HC_LOGGING

#ifndef HC_HEADLESS
    if (!glfwInit()) {
        const char *description;
        glfwGetError(&description);
        HC_ERROR("Error initialising GLFW's context: %s\n" << description);
        return -1;
    }

    glfwSetErrorCallback(glfw_error_callback);
#endif // HC_HEADLESS

    return 0;
}

int hc_term() {
#ifndef HC_HEADLESS
    glfwTerminate();
#endif // HC_HEADLESS

    return 0;
}

int hc_render_tick() {
    return 0;
}

#ifndef HC_HEADLESS

void hc_poll_events() {
    glfwPollEvents();
}

#endif // HC_HEADLESS
