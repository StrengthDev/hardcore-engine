
#include <pch.hpp>

#ifndef HC_HEADLESS

#include "window.hpp"

#include "context.hpp"

#include "../core/error.hpp"
#include "../core/log.hpp"

#include <core/window.h>
#include <render/renderer.hpp>

#include <util/number.hpp>

namespace hc::window {
    static inline HCDeviceEvent from_glfw_event(int event) {
        switch (event) {
        case GLFW_CONNECTED:
            return HCDeviceEvent_Connected;
        case GLFW_DISCONNECTED:
            return HCDeviceEvent_Disconnected;
        default: HC_UNREACHABLE("All possible values must be handled");
        }
    }

    std::expected<Window, Error> Window::create(
        ContextParams const& context_params,
        HCWindowParams const& window_params
    ) {
        Window window;

        window.window_id = context_params.window_id;
        window.owning_device = window_params.device;

        auto device_result = render::device_at(window_params.device);
        if (!device_result) {
            return device_result.error();
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (window_params.pos_x == std::numeric_limits<int>::max()) {
            glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
        } else {
            glfwWindowHint(GLFW_POSITION_X, window_params.pos_x);
        }

        if (window_params.pos_y == std::numeric_limits<int>::max()) {
            glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
        } else {
            glfwWindowHint(GLFW_POSITION_Y, window_params.pos_y);
        }

        window.glfw_handle = glfwCreateWindow(
            static_cast<int>(window_params.width),
            static_cast<int>(window_params.height),
            window_params.name,
            nullptr,
            nullptr
        );

        if (!window.glfw_handle) {
            const char* description;
            glfwGetError(&description);
            HC_ERROR("Failed to create window: " << description);
            return Error(HCError_GLFWWindowCreationFailure);
        }

        glfwSetInputMode(window.glfw_handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

        if (context_params.raw_mouse_input_available) {
            glfwSetInputMode(window.glfw_handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        window.set_framebuffer_callback(nullptr);

        int width, height;
        glfwGetFramebufferSize(window.glfw_handle, &width, &height);
        window.extent.width = static_cast<u32>(width);
        window.extent.height = static_cast<u32>(height);

        auto swapchain_result = (*device_result)->create_swapchain(window.glfw_handle, window.extent);
        if (!swapchain_result) {
            return swapchain_result.error();
        }

        HC_INFO("Created new window with id " << window.window_id);

        return window;
    }

    Window::~Window() {
        if (this->glfw_handle) {
            HC_INFO("Destroying window " << this->window_id << " (handle: " << this->glfw_handle << ')');
            glfwDestroyWindow(this->glfw_handle);
        }
    }

    void Window::disable() noexcept {
        glfwSetWindowPosCallback(this->glfw_handle, nullptr);
        glfwSetWindowSizeCallback(this->glfw_handle, nullptr);
        glfwSetWindowCloseCallback(this->glfw_handle, nullptr);
        glfwSetWindowRefreshCallback(this->glfw_handle, nullptr);
        glfwSetWindowFocusCallback(this->glfw_handle, nullptr);
        glfwSetWindowIconifyCallback(this->glfw_handle, nullptr);
        glfwSetWindowMaximizeCallback(this->glfw_handle, nullptr);
        glfwSetFramebufferSizeCallback(this->glfw_handle, nullptr);
        glfwSetWindowContentScaleCallback(this->glfw_handle, nullptr);
        glfwSetMouseButtonCallback(this->glfw_handle, nullptr);
        glfwSetCursorPosCallback(this->glfw_handle, nullptr);
        glfwSetCursorEnterCallback(this->glfw_handle, nullptr);
        glfwSetScrollCallback(this->glfw_handle, nullptr);
        glfwSetKeyCallback(this->glfw_handle, nullptr);
        glfwSetCharCallback(this->glfw_handle, nullptr);
        glfwSetCharModsCallback(this->glfw_handle, nullptr);
        glfwSetDropCallback(this->glfw_handle, nullptr);

        int x, y;
        glfwGetWindowPos(this->glfw_handle, &x, &y);

        glfwSetWindowMonitor(
            this->glfw_handle,
            nullptr,
            x,
            y,
            static_cast<int>(this->extent.width),
            static_cast<int>(this->extent.height),
            0
        );

        glfwHideWindow(this->glfw_handle);

        render::device_list()[this->owning_device].destroy_swapchain(this->glfw_handle);
    }

    u64 Window::id() const noexcept {
        return this->window_id;
    }

    GLFWwindow const* Window::handle() const noexcept {
        return this->glfw_handle;
    }

    std::expected<void, Error> Window::set_cursor_mode(HCCursorMode cursor_mode) noexcept {
        int mode;
        switch (cursor_mode) {
        case HCCursorMode_Normal:
            HC_TRACE("Setting cursor mode normal for window " << this->window_id);
            mode = GLFW_CURSOR_NORMAL;
            break;
        case HCCursorMode_Hidden:
            HC_TRACE("Setting cursor mode hidden for window " << this->window_id);
            mode = GLFW_CURSOR_HIDDEN;
            break;
        case HCCursorMode_Captured:
            HC_TRACE("Setting cursor mode captured for window " << this->window_id);
            mode = GLFW_CURSOR_CAPTURED;
            break;
        case HCCursorMode_Disabled:
            HC_TRACE("Setting cursor mode disabled for window " << this->window_id);
            mode = GLFW_CURSOR_DISABLED;
            break;
        default:
            HC_ERROR("Invalid cursor mode value.");
            return Error(HCError_InvalidParams);
        }

        glfwSetInputMode(this->glfw_handle, GLFW_CURSOR, mode);

        return {};
    }

    void Window::set_position_callback(HCWindowPositionCallback callback) noexcept {
        this->position_callback = callback;

        if (callback) {
            HC_TRACE("Setting window position callback for window " << this->window_id);
            glfwSetWindowPosCallback(
                this->glfw_handle,
                [](GLFWwindow* glfw_window, int width, int height) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.position_callback(window.window_id, width, height);
                }
            );
        } else {
            HC_TRACE("Unsetting window position callback for window " << this->window_id);
            glfwSetWindowPosCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_size_callback(HCWindowSizeCallback callback) noexcept {
        this->size_callback = callback;

        if (callback) {
            HC_TRACE("Setting window size callback for window " << this->window_id);
            glfwSetWindowSizeCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int width, int height) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.size_callback(window.window_id, width, height);
                }
            );
        } else {
            HC_TRACE("Unsetting window size callback for window " << this->window_id);
            glfwSetWindowSizeCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_close_callback(HCWindowCloseCallback callback) noexcept {
        this->close_callback = callback;

        if (callback) {
            HC_TRACE("Setting window close callback for window " << this->window_id);
            glfwSetWindowCloseCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.close_callback(window.window_id);
                }
            );
        } else {
            HC_TRACE("Unsetting window close callback for window " << this->window_id);
            glfwSetWindowCloseCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_refresh_callback(HCWindowRefreshCallback callback) noexcept {
        this->refresh_callback = callback;

        if (callback) {
            HC_TRACE("Setting window refresh callback for window " << this->window_id);
            glfwSetWindowRefreshCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.refresh_callback(window.window_id);
                }
            );
        } else {
            HC_TRACE("Unsetting window refresh callback for window " << this->window_id);
            glfwSetWindowRefreshCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_focus_callback(HCWindowFocusCallback callback) noexcept {
        this->focus_callback = callback;

        if (callback) {
            HC_TRACE("Setting window focus callback for window " << this->window_id);
            glfwSetWindowFocusCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int focused) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.focus_callback(window.window_id, focused == GLFW_TRUE);
                }
            );
        } else {
            HC_TRACE("Unsetting window focus callback for window " << this->window_id);
            glfwSetWindowFocusCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_minimize_callback(HCWindowMinimizeCallback callback) noexcept {
        this->minimize_callback = callback;

        if (callback) {
            HC_TRACE("Setting window minimize callback for window " << this->window_id);
            glfwSetWindowIconifyCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int minimized) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.minimize_callback(window.window_id, minimized == GLFW_TRUE);
                }
            );
        } else {
            HC_TRACE("Unsetting window minimize callback for window " << this->window_id);
            glfwSetWindowIconifyCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_maximize_callback(HCWindowMaximizeCallback callback) noexcept {
        this->maximize_callback = callback;

        if (callback) {
            HC_TRACE("Setting window maximize callback for window " << this->window_id);
            glfwSetWindowMaximizeCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int maximized) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.maximize_callback(window.window_id, maximized == GLFW_TRUE);
                }
            );
        } else {
            HC_TRACE("Unsetting window maximize callback for window " << this->window_id);
            glfwSetWindowMaximizeCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_framebuffer_callback(HCWindowFramebufferCallback callback) noexcept {
        this->framebuffer_callback = callback;

        if (callback) {
            HC_TRACE("Setting window framebuffer callback for window " << this->window_id);
            glfwSetFramebufferSizeCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int width, int height) {
                    Window& window = Context::instance().get_window(glfw_window);

                    window.resize(width, height);
                    window.framebuffer_callback(window.window_id, width, height);
                }
            );
        } else {
            HC_TRACE("Unsetting window framebuffer callback for window " << this->window_id);
            glfwSetFramebufferSizeCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int width, int height) {
                    Window& window = Context::instance().get_window(glfw_window);
                    window.resize(width, height);
                }
            );
        }
    }

    void Window::set_scale_callback(HCWindowScaleCallback callback) noexcept {
        this->scale_callback = callback;

        if (callback) {
            HC_TRACE("Setting window scale callback for window " << this->window_id);
            glfwSetWindowContentScaleCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, float x_scale, float y_scale) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.scale_callback(window.window_id, x_scale, y_scale);
                }
            );
        } else {
            HC_TRACE("Unsetting window scale callback for window " << this->window_id);
            glfwSetWindowContentScaleCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_mouse_button_callback(HCWindowMouseButtonCallback callback) noexcept {
        this->mouse_button_callback = callback;

        if (callback) {
            HC_TRACE("Setting mouse button callback for window " << this->window_id);
            glfwSetMouseButtonCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int button, int action, int mods) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.mouse_button_callback(
                        window.window_id,
                        static_cast<HCMouseButton>(button),
                        static_cast<HCButtonAction>(action),
                        mods
                    );
                }
            );
        } else {
            HC_TRACE("Unsetting mouse button callback for window " << this->window_id);
            glfwSetMouseButtonCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_cursor_position_callback(HCWindowCursorPositionCallback callback) noexcept {
        this->cursor_position_callback = callback;

        if (callback) {
            HC_TRACE("Setting cursor position callback for window " << this->window_id);
            glfwSetCursorPosCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, double x, double y) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.cursor_position_callback(window.window_id, x, y);
                }
            );
        } else {
            HC_TRACE("Unsetting cursor position callback for window " << this->window_id);
            glfwSetCursorPosCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_cursor_enter_callback(HCWindowCursorEnterCallback callback) noexcept {
        this->cursor_enter_callback = callback;

        if (callback) {
            HC_TRACE("Setting cursor enter callback for window " << this->window_id);
            glfwSetCursorEnterCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int entered) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.cursor_enter_callback(window.window_id, entered == GLFW_TRUE);
                }
            );
        } else {
            HC_TRACE("Unsetting cursor enter callback for window " << this->window_id);
            glfwSetCursorEnterCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_scroll_callback(HCWindowScrollCallback callback) noexcept {
        this->scroll_callback = callback;

        if (callback) {
            HC_TRACE("Setting scroll callback for window " << this->window_id);
            glfwSetScrollCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, double x_offset, double y_offset) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.scroll_callback(window.window_id, x_offset, y_offset);
                }
            );
        } else {
            HC_TRACE("Unsetting scroll callback for window " << this->window_id);
            glfwSetScrollCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_key_callback(HCWindowKeyCallback callback) noexcept {
        this->key_callback = callback;

        if (callback) {
            HC_TRACE("Setting key callback for window " << this->window_id);
            glfwSetKeyCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int key, int scan_code, int action, int mods) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.key_callback(
                        window.window_id,
                        static_cast<HCKeyboardKey>(key),
                        scan_code,
                        static_cast<HCButtonAction>(action),
                        mods
                    );
                }
            );
        } else {
            HC_TRACE("Unsetting key callback for window " << this->window_id);
            glfwSetKeyCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_char_callback(HCWindowCharCallback callback) noexcept {
        this->char_callback = callback;

        if (callback) {
            HC_TRACE("Setting character callback for window " << this->window_id);
            glfwSetCharCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, unsigned int code_point) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.char_callback(window.window_id, code_point);
                }
            );
        } else {
            HC_TRACE("Unsetting character callback for window " << this->window_id);
            glfwSetCharCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_char_mods_callback(HCWindowCharModsCallback callback) noexcept {
        this->char_mod_callback = callback;

        if (callback) {
            HC_TRACE("Setting character with mods callback for window " << this->window_id);
            glfwSetCharModsCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, unsigned int code_point, int mods) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.char_mod_callback(window.window_id, code_point, mods);
                }
            );
        } else {
            HC_TRACE("Unsetting character with mods callback for window " << this->window_id);
            glfwSetCharModsCallback(glfw_handle, nullptr);
        }
    }

    void Window::set_drop_callback(HCWindowDropCallback callback) noexcept {
        this->drop_callback = callback;

        if (callback) {
            HC_TRACE("Setting drop callback for window " << this->window_id);
            glfwSetDropCallback(
                glfw_handle,
                [](GLFWwindow* glfw_window, int path_count, const char* paths[]) {
                    Window const& window = Context::instance().get_window(glfw_window);
                    window.drop_callback(window.window_id, path_count, paths);
                }
            );
        } else {
            HC_TRACE("Unsetting drop callback for window " << this->window_id);
            glfwSetDropCallback(glfw_handle, nullptr);
        }
    }

    void Window::finish_resize() {
        this->resizing = false;
    }

    void Window::resize(int width, int height) {
        // Set resizing to true to keep the window's swapchain from recreating itself, while glfwPollEvents is blocking.
        // This way, swapchains are only recreated at the end of glfwPollEvents, when the user stops
        // resizing the window.
        this->resizing = true;

        this->extent.width = static_cast<u32>(width);
        this->extent.height = static_cast<u32>(height);

        render::device_list()[this->owning_device].resize_framebuffer(this->glfw_handle, this->extent);
    }
}

#endif // HC_HEADLESS
