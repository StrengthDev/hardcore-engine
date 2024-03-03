#include <pch.hpp>

#ifndef HEADLESS

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#endif // HEADLESS

#include <core/core.h>

int hc_init() {
#ifndef HEADLESS
    if (glfwInit()) {
        const char *description;
        glfwGetError(&description);
//        printf("Error: %s\n", description); TODO
        return -1;
    }

    //glfwSetErrorCallback(errorCallback); TODO
#endif // HEADLESS

    return 0;
}

int hc_term() {
#ifndef HEADLESS
    glfwTerminate();
#endif // HEADLESS

    return 0;
}

