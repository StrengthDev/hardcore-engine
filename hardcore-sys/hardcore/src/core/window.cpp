#include <pch.hpp>

#ifndef HEADLESS

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/window.h>

HCWindow hc_new_window() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(1080, 720, "", nullptr, nullptr);

    return HCWindow{.handle = window};
}

void hc_destroy_window(HCWindow *window) {
    if (window->handle) {
        glfwDestroyWindow(static_cast<GLFWwindow *>(window->handle));
        window->handle = nullptr;
    }
}

#endif // HEADLESS
