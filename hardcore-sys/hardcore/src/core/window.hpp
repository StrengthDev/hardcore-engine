#pragma once

#ifndef HC_HEADLESS

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

namespace hc::window {
    void destroy(GLFWwindow *window);

    bool is_resizing(GLFWwindow *window);
}

#endif // HC_HEADLESS
