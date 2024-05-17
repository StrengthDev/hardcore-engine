#pragma once

#include <variant>

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/util.hpp>

namespace hc::render::device {
    struct WindowDestructionMark {
        VkInstance instance;
        GLFWwindow *window;
    };

    struct SwapchainDestructionMark {
        GLFWwindow *window;
    };

    /**
     * @brief A variant which may contain any of the deletion marks.
     */
    typedef std::variant<WindowDestructionMark, SwapchainDestructionMark> DestructionMark;

    /**
     * @brief A helper type used vor visiting a variant.
     *
     * @tparam Ts The types of the function overloads, these are typically inferred from the visit call. A function
     * which deduces it's argument using `auto` may be used for a "default" handler.
     */
    template<class... Ts>
    struct DestructionMarkHandler : Ts ... {
        using Ts::operator()...;
    };
}
