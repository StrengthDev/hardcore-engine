
#pragma once

#include <core/glfw.hpp>

namespace hc::window {
    class Handle {
    public:
        Handle() = default;

        Handle(GLFWwindow* value) noexcept;

        Handle(Handle&& other) noexcept;

        Handle& operator=(Handle&& other) noexcept;

        operator GLFWwindow*() const noexcept;

    private:
        GLFWwindow* value = nullptr;
    };
};
