
#include <pch.hpp>

#ifndef HC_HEADLESS

#include <core/window.h>

#include "../validation.hpp"

#include <window/context.hpp>
#include <window/window.hpp>

#include <core/error.hpp>
#include <core/log.hpp>

#include <util/function_signature.hpp>

const HCVersion HC_GLFW_VERSION = { GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION };

void hc_poll_events() {
    hc::window::Context::instance().poll_events();
}

HCResult hc_new_window(HCWindow* window, HCWindowParams params) {
    HC_VALIDATE_PTR_RE(window, "window");

    auto window_result = hc::window::Context::instance().create_window(params);
    if (!window_result) {
        return window_result.error();
    }

    window->id = *window_result;

    return { .success = true };
}

void hc_destroy_window(HCWindow* window) {
    HC_VALIDATE_PTR(window, "window");

    hc::window::Context::instance().yield_window(window->id);

    *window = {};
}

template<typename S, typename A>
concept SimpleWindowSetter = requires(hc::window::Window window, S setter, A arg) { { (window.*setter)(arg) } -> std::convertible_to<void>; }
    || requires(hc::window::Window window, S setter, A arg) { { (window.*setter)(arg) } -> std::convertible_to<std::expected<void, hc::Error>>; };

template<typename A, SimpleWindowSetter<A> S>
static HCResult call_window_setter(HCWindow* window, S setter, A arg) {
    HC_VALIDATE_PTR_RE(window, "window");

    auto window_obj = hc::window::Context::instance().find_window(window->id);
    if (!window_obj) {
        HC_WARN("No such window: " << window->id);
        return hc::Error(HCError_InvalidParams);
    }

    if constexpr (std::is_same_v<typename FunctionSignature<S>::Return, void>) {
        (window_obj->*setter)(arg);
    } else {
        auto result = (window_obj->*setter)(arg);
        if (!result) {
            return result.error();
        }
    }

    return { .success = true };
}

HCResult hc_set_window_cursor_mode(HCWindow* window, HCCursorMode cursor_mode) {
    return call_window_setter(window, &hc::window::Window::set_cursor_mode, cursor_mode);
}

HCResult hc_set_window_position_callback(HCWindow* window, HCWindowPositionCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_position_callback, callback);
}

HCResult hc_set_window_size_callback(HCWindow* window, HCWindowSizeCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_size_callback, callback);
}

HCResult hc_set_window_close_callback(HCWindow* window, HCWindowCloseCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_close_callback, callback);
}

HCResult hc_set_window_refresh_callback(HCWindow* window, HCWindowRefreshCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_refresh_callback, callback);
}

HCResult hc_set_window_focus_callback(HCWindow* window, HCWindowFocusCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_focus_callback, callback);
}

HCResult hc_set_window_minimize_callback(HCWindow* window, HCWindowMinimizeCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_minimize_callback, callback);
}

HCResult hc_set_window_maximize_callback(HCWindow* window, HCWindowMaximizeCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_maximize_callback, callback);
}

HCResult hc_set_window_framebuffer_callback(HCWindow* window, HCWindowFramebufferCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_framebuffer_callback, callback);
}

HCResult hc_set_window_scale_callback(HCWindow* window, HCWindowScaleCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_scale_callback, callback);
}

HCResult hc_set_window_mouse_button_callback(HCWindow* window, HCWindowMouseButtonCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_mouse_button_callback, callback);
}

HCResult hc_set_window_cursor_position_callback(HCWindow* window, HCWindowCursorPositionCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_cursor_position_callback, callback);
}

HCResult hc_set_window_cursor_enter_callback(HCWindow* window, HCWindowCursorEnterCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_cursor_enter_callback, callback);
}

HCResult hc_set_window_scroll_callback(HCWindow* window, HCWindowScrollCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_scroll_callback, callback);
}

HCResult hc_set_window_key_callback(HCWindow* window, HCWindowKeyCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_key_callback, callback);
}

HCResult hc_set_window_char_callback(HCWindow* window, HCWindowCharCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_char_callback, callback);
}

HCResult hc_set_window_char_mods_callback(HCWindow* window, HCWindowCharModsCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_char_mods_callback, callback);
}

HCResult hc_set_window_drop_callback(HCWindow* window, HCWindowDropCallback callback) {
    return call_window_setter(window, &hc::window::Window::set_drop_callback, callback);
}

#endif // HC_HEADLESS
