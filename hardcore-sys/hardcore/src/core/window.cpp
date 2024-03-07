#include <pch.hpp>

#ifndef HC_HEADLESS

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/window.h>
#include <core/log.hpp>

HCWindow hc_new_window() {
    HC_INFO("Creating new window: " << "Placeholder");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(1080, 720, "Placeholder", nullptr, nullptr);

    return HCWindow{.handle = window};
}

void hc_destroy_window(HCWindow *window) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        const char *name = glfwGetWindowTitle(handle);
        if (name) {
            HC_INFO("Destroying window: " << glfwGetWindowTitle(static_cast<GLFWwindow *>(window->handle)));
            glfwDestroyWindow(handle);
            window->handle = nullptr;
        } else {
            HC_DEBUG("Invalid window or GLFW context");
        }
    }
}

#endif // HC_HEADLESS
