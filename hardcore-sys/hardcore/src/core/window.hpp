#pragma once

#ifndef HC_HEADLESS

#include <core/glfw.hpp>

namespace hc::window {
    bool init_context();

    void terminate_context();

    void destroy(GLFWwindow* window);

    bool is_resizing(GLFWwindow* window);

    VkExtent2D extent(GLFWwindow* window);
}

#endif // HC_HEADLESS
