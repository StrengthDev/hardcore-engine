#include <pch.hpp>

#ifndef HC_HEADLESS
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#endif // HC_HEADLESS

#include <core/core.h>
#include "log.hpp"

int hc_init(HCInitParams params) {
#ifdef HC_LOGGING
    if (params.log_fn) {
        hc::set_log(params.log_fn);
    }
#endif // HC_LOGGING

#ifndef HC_HEADLESS
    if (!glfwInit()) {
        const char *description;
        glfwGetError(&description);
        ERROR("Error initialising GLFW's context: %s\n" << description);
        return -1;
    }

    //glfwSetErrorCallback(errorCallback); TODO
#endif // HC_HEADLESS

    return 0;
}

int hc_term() {
#ifndef HC_HEADLESS
    glfwTerminate();
#endif // HC_HEADLESS

    return 0;
}
