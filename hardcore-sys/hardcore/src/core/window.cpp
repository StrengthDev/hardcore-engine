#include <pch.hpp>

#ifndef HC_HEADLESS

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/window.h>
#include <core/window.hpp>
#include <core/log.hpp>
#include <util/number.hpp>
#include <render/renderer.hpp>

namespace hc {
    inline HCMouseButton from_glfw_button(int button) {
#ifndef HC_UNSAFE_TYPE_CASTS
        switch (button) {
            case GLFW_MOUSE_BUTTON_1:
                return HCMouseButton::Button1;
            case GLFW_MOUSE_BUTTON_2:
                return HCMouseButton::Button2;
            case GLFW_MOUSE_BUTTON_3:
                return HCMouseButton::Button3;
            case GLFW_MOUSE_BUTTON_4:
                return HCMouseButton::Button4;
            case GLFW_MOUSE_BUTTON_5:
                return HCMouseButton::Button5;
            case GLFW_MOUSE_BUTTON_6:
                return HCMouseButton::Button6;
            case GLFW_MOUSE_BUTTON_7:
                return HCMouseButton::Button7;
            case GLFW_MOUSE_BUTTON_8:
                return HCMouseButton::Button8;
            default:
                return HCMouseButton{};
        }
#else
        return static_cast<HCMouseButton>(button);
#endif // HC_UNSAFE_TYPE_CASTS
    }

    inline HCButtonAction from_glfw_action(int action) {
#ifndef HC_UNSAFE_TYPE_CASTS
        switch (action) {
            case GLFW_RELEASE:
                return HCButtonAction::Release;
            case GLFW_PRESS:
                return HCButtonAction::Press;
            case GLFW_REPEAT:
                return HCButtonAction::Repeat;
            default:
                return HCButtonAction{};
        }
#else
        return static_cast<HCButtonAction>(action);
#endif // HC_UNSAFE_TYPE_CASTS
    }

    inline HCKeyboardKey from_glfw_key(int key) {
#ifndef HC_UNSAFE_TYPE_CASTS
        switch (key) {
            case GLFW_KEY_SPACE:
                return HCKeyboardKey::Space;
            case GLFW_KEY_APOSTROPHE:
                return HCKeyboardKey::Apostrophe;
            case GLFW_KEY_COMMA:
                return HCKeyboardKey::Comma;
            case GLFW_KEY_MINUS:
                return HCKeyboardKey::Minus;
            case GLFW_KEY_PERIOD:
                return HCKeyboardKey::Period;
            case GLFW_KEY_SLASH:
                return HCKeyboardKey::Slash;
            case GLFW_KEY_0:
                return HCKeyboardKey::Num0;
            case GLFW_KEY_1:
                return HCKeyboardKey::Num1;
            case GLFW_KEY_2:
                return HCKeyboardKey::Num2;
            case GLFW_KEY_3:
                return HCKeyboardKey::Num3;
            case GLFW_KEY_4:
                return HCKeyboardKey::Num4;
            case GLFW_KEY_5:
                return HCKeyboardKey::Num5;
            case GLFW_KEY_6:
                return HCKeyboardKey::Num6;
            case GLFW_KEY_7:
                return HCKeyboardKey::Num7;
            case GLFW_KEY_8:
                return HCKeyboardKey::Num8;
            case GLFW_KEY_9:
                return HCKeyboardKey::Num9;
            case GLFW_KEY_SEMICOLON:
                return HCKeyboardKey::Semicolon;
            case GLFW_KEY_EQUAL:
                return HCKeyboardKey::Equal;
            case GLFW_KEY_A:
                return HCKeyboardKey::A;
            case GLFW_KEY_B:
                return HCKeyboardKey::B;
            case GLFW_KEY_C:
                return HCKeyboardKey::C;
            case GLFW_KEY_D:
                return HCKeyboardKey::D;
            case GLFW_KEY_E:
                return HCKeyboardKey::E;
            case GLFW_KEY_F:
                return HCKeyboardKey::F;
            case GLFW_KEY_G:
                return HCKeyboardKey::G;
            case GLFW_KEY_H:
                return HCKeyboardKey::H;
            case GLFW_KEY_I:
                return HCKeyboardKey::I;
            case GLFW_KEY_J:
                return HCKeyboardKey::J;
            case GLFW_KEY_K:
                return HCKeyboardKey::K;
            case GLFW_KEY_L:
                return HCKeyboardKey::L;
            case GLFW_KEY_M:
                return HCKeyboardKey::M;
            case GLFW_KEY_N:
                return HCKeyboardKey::N;
            case GLFW_KEY_O:
                return HCKeyboardKey::O;
            case GLFW_KEY_P:
                return HCKeyboardKey::P;
            case GLFW_KEY_Q:
                return HCKeyboardKey::Q;
            case GLFW_KEY_R:
                return HCKeyboardKey::R;
            case GLFW_KEY_S:
                return HCKeyboardKey::S;
            case GLFW_KEY_T:
                return HCKeyboardKey::T;
            case GLFW_KEY_U:
                return HCKeyboardKey::U;
            case GLFW_KEY_V:
                return HCKeyboardKey::V;
            case GLFW_KEY_W:
                return HCKeyboardKey::W;
            case GLFW_KEY_X:
                return HCKeyboardKey::X;
            case GLFW_KEY_Y:
                return HCKeyboardKey::Y;
            case GLFW_KEY_Z:
                return HCKeyboardKey::Z;
            case GLFW_KEY_LEFT_BRACKET:
                return HCKeyboardKey::LeftBracket;
            case GLFW_KEY_BACKSLASH:
                return HCKeyboardKey::Backslash;
            case GLFW_KEY_RIGHT_BRACKET:
                return HCKeyboardKey::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT:
                return HCKeyboardKey::GraveAccent;
            case GLFW_KEY_WORLD_1:
                return HCKeyboardKey::World1;
            case GLFW_KEY_WORLD_2:
                return HCKeyboardKey::World2;
            case GLFW_KEY_ESCAPE:
                return HCKeyboardKey::Escape;
            case GLFW_KEY_ENTER:
                return HCKeyboardKey::Enter;
            case GLFW_KEY_TAB:
                return HCKeyboardKey::Tab;
            case GLFW_KEY_BACKSPACE:
                return HCKeyboardKey::Backspace;
            case GLFW_KEY_INSERT:
                return HCKeyboardKey::Insert;
            case GLFW_KEY_DELETE:
                return HCKeyboardKey::Delete;
            case GLFW_KEY_RIGHT:
                return HCKeyboardKey::Right;
            case GLFW_KEY_LEFT:
                return HCKeyboardKey::Left;
            case GLFW_KEY_DOWN:
                return HCKeyboardKey::Down;
            case GLFW_KEY_UP:
                return HCKeyboardKey::Up;
            case GLFW_KEY_PAGE_UP:
                return HCKeyboardKey::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return HCKeyboardKey::PageDown;
            case GLFW_KEY_HOME:
                return HCKeyboardKey::Home;
            case GLFW_KEY_END:
                return HCKeyboardKey::End;
            case GLFW_KEY_CAPS_LOCK:
                return HCKeyboardKey::CapsLock;
            case GLFW_KEY_SCROLL_LOCK:
                return HCKeyboardKey::ScrollLock;
            case GLFW_KEY_NUM_LOCK:
                return HCKeyboardKey::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return HCKeyboardKey::PrintScreen;
            case GLFW_KEY_PAUSE:
                return HCKeyboardKey::Pause;
            case GLFW_KEY_F1:
                return HCKeyboardKey::F1;
            case GLFW_KEY_F2:
                return HCKeyboardKey::F2;
            case GLFW_KEY_F3:
                return HCKeyboardKey::F3;
            case GLFW_KEY_F4:
                return HCKeyboardKey::F4;
            case GLFW_KEY_F5:
                return HCKeyboardKey::F5;
            case GLFW_KEY_F6:
                return HCKeyboardKey::F6;
            case GLFW_KEY_F7:
                return HCKeyboardKey::F7;
            case GLFW_KEY_F8:
                return HCKeyboardKey::F8;
            case GLFW_KEY_F9:
                return HCKeyboardKey::F9;
            case GLFW_KEY_F10:
                return HCKeyboardKey::F10;
            case GLFW_KEY_F11:
                return HCKeyboardKey::F11;
            case GLFW_KEY_F12:
                return HCKeyboardKey::F12;
            case GLFW_KEY_F13:
                return HCKeyboardKey::F13;
            case GLFW_KEY_F14:
                return HCKeyboardKey::F14;
            case GLFW_KEY_F15:
                return HCKeyboardKey::F15;
            case GLFW_KEY_F16:
                return HCKeyboardKey::F16;
            case GLFW_KEY_F17:
                return HCKeyboardKey::F17;
            case GLFW_KEY_F18:
                return HCKeyboardKey::F18;
            case GLFW_KEY_F19:
                return HCKeyboardKey::F19;
            case GLFW_KEY_F20:
                return HCKeyboardKey::F20;
            case GLFW_KEY_F21:
                return HCKeyboardKey::F21;
            case GLFW_KEY_F22:
                return HCKeyboardKey::F22;
            case GLFW_KEY_F23:
                return HCKeyboardKey::F23;
            case GLFW_KEY_F24:
                return HCKeyboardKey::F24;
            case GLFW_KEY_F25:
                return HCKeyboardKey::F25;
            case GLFW_KEY_KP_0:
                return HCKeyboardKey::Numpad0;
            case GLFW_KEY_KP_1:
                return HCKeyboardKey::Numpad1;
            case GLFW_KEY_KP_2:
                return HCKeyboardKey::Numpad2;
            case GLFW_KEY_KP_3:
                return HCKeyboardKey::Numpad3;
            case GLFW_KEY_KP_4:
                return HCKeyboardKey::Numpad4;
            case GLFW_KEY_KP_5:
                return HCKeyboardKey::Numpad5;
            case GLFW_KEY_KP_6:
                return HCKeyboardKey::Numpad6;
            case GLFW_KEY_KP_7:
                return HCKeyboardKey::Numpad7;
            case GLFW_KEY_KP_8:
                return HCKeyboardKey::Numpad8;
            case GLFW_KEY_KP_9:
                return HCKeyboardKey::Numpad9;
            case GLFW_KEY_KP_DECIMAL:
                return HCKeyboardKey::NumpadDecimal;
            case GLFW_KEY_KP_DIVIDE:
                return HCKeyboardKey::NumpadDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return HCKeyboardKey::NumpadMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return HCKeyboardKey::NumpadSubtract;
            case GLFW_KEY_KP_ADD:
                return HCKeyboardKey::NumpadAdd;
            case GLFW_KEY_KP_ENTER:
                return HCKeyboardKey::NumpadEnter;
            case GLFW_KEY_KP_EQUAL:
                return HCKeyboardKey::NumpadEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return HCKeyboardKey::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return HCKeyboardKey::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return HCKeyboardKey::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return HCKeyboardKey::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return HCKeyboardKey::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return HCKeyboardKey::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return HCKeyboardKey::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return HCKeyboardKey::RightSuper;
            case GLFW_KEY_MENU:
                return HCKeyboardKey::Menu;
            default:
                return HCKeyboardKey{};
        }
#else
        return static_cast<HCKeyboardKey>(key);
#endif // HC_UNSAFE_TYPE_CASTS
    }

    inline HCDeviceEvent from_glfw_event(int event) {
#ifndef HC_UNSAFE_TYPE_CASTS
        switch (event) {
            case GLFW_CONNECTED:
                return HCDeviceEvent::Connected;
            case GLFW_DISCONNECTED:
                return HCDeviceEvent::Disconnected;
            default:
                return HCDeviceEvent{};
        }
#else
        return static_cast<HCDeviceEvent>(event);
#endif // HC_UNSAFE_TYPE_CASTS
    }
}

/**
 * @brief Static window callback function pointers.
 */
struct StaticWindow {
    Sz id = std::numeric_limits<Sz>::max();
    u32 owning_device = std::numeric_limits<u32>::max();
    bool resizing = false;
    HCWindowPositionCallback position_callback = nullptr;
    HCWindowSizeCallback size_callback = nullptr;
    HCWindowCloseCallback close_callback = nullptr;
    HCWindowRefreshCallback refresh_callback = nullptr;
    HCWindowFocusCallback focus_callback = nullptr;
    HCWindowMinimizeCallback minimize_callback = nullptr;
    HCWindowMaximizeCallback maximize_callback = nullptr;
    HCWindowFramebufferCallback framebuffer_callback = nullptr;
    HCWindowScaleCallback scale_callback = nullptr;
    HCWindowMouseButtonCallback mouse_button_callback = nullptr;
    HCWindowCursorPositionCallback cursor_position_callback = nullptr;
    HCWindowCursorEnterCallback cursor_enter_callback = nullptr;
    HCWindowScrollCallback scroll_callback = nullptr;
    HCWindowKeyCallback key_callback = nullptr;
    HCWindowCharCallback char_callback = nullptr;
    HCWindowCharModsCallback char_mod_callback = nullptr;
    HCWindowDropCallback drop_callback = nullptr;
};

/**
 * @brief The mutex used to lock access to `window_map`.
 */
static std::shared_mutex window_mutex;

/**
 * @brief A map that takes a GLFWwindow pointer as a key, and stores the respective window callbacks.
 *
 * This is needed because GLFW window callbacks do not accept user data and only provide the GLFWwindow pointer within
 * the callback, so this is used to access each window's individual callbacks.
 */
static std::unordered_map<void *, StaticWindow> window_map;

void hc_poll_events() {
    glfwPollEvents();

    // Unsure if this needed, as callbacks should only be called from within glfwPollEvents.
    std::unique_lock lock(window_mutex);
    // Set resizing to false, in order to allow swapchains to be recreated.
    for (auto &[window, static_window]: window_map) {
        static_window.resizing = false;
    }
}

HCWindow hc_new_window(HCWindowParams params) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (params.pos_x == std::numeric_limits<int>::max()) {
        glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
    } else {
        glfwWindowHint(GLFW_POSITION_X, params.pos_x);
    }
    if (params.pos_y == std::numeric_limits<int>::max()) {
        glfwWindowHint(GLFW_POSITION_X, GLFW_ANY_POSITION);
    } else {
        glfwWindowHint(GLFW_POSITION_Y, params.pos_y);
    }

    GLFWwindow *window = glfwCreateWindow(static_cast<int>(params.width), static_cast<int>(params.height), params.name,
                                          nullptr, nullptr);

    if (!window) {
        HC_ERROR("Failed to create window");
        return HCWindow{.handle = nullptr, .id = 0};
    }

    u32 device = hc::render::default_device();
    hc::render::InstanceResult res = hc::render::create_swapchain(window, device);
    if (res != hc::render::InstanceResult::Success) {
        HC_ERROR("Failed to create swapchain");
        glfwDestroyWindow(window);
        return HCWindow{.handle = nullptr, .id = 0};
    }

    std::unique_lock lock(window_mutex);
    std::vector<Sz> ids;
    ids.reserve(window_map.size());
    for (auto item: window_map) {
        ids.push_back(item.second.id);
    }
    std::sort(ids.begin(), ids.end());
    Sz id = 0;
    for (auto item: ids) {
        if (id == item) {
            id++;
        } else {
            break;
        }
    }

    StaticWindow static_window;
    static_window.id = id;
    static_window.owning_device = device;
    window_map.emplace(window, static_window);

    HC_INFO("Created new window with id " << id);

    return HCWindow{.handle = window, .id = id};
}

void hc_destroy_window(HCWindow *window) {
    if (!window || !window->handle)
        return;

    HC_INFO("Window " << window->id << " marked for destruction (handle: " << window->handle << ')');
    auto *handle = static_cast<GLFWwindow *>(window->handle);

    glfwSetWindowPosCallback(handle, nullptr);
    glfwSetWindowSizeCallback(handle, nullptr);
    glfwSetWindowCloseCallback(handle, nullptr);
    glfwSetWindowRefreshCallback(handle, nullptr);
    glfwSetWindowFocusCallback(handle, nullptr);
    glfwSetWindowIconifyCallback(handle, nullptr);
    glfwSetWindowMaximizeCallback(handle, nullptr);
    glfwSetFramebufferSizeCallback(handle, nullptr);
    glfwSetWindowContentScaleCallback(handle, nullptr);
    glfwSetMouseButtonCallback(handle, nullptr);
    glfwSetCursorPosCallback(handle, nullptr);
    glfwSetCursorEnterCallback(handle, nullptr);
    glfwSetScrollCallback(handle, nullptr);
    glfwSetKeyCallback(handle, nullptr);
    glfwSetCharCallback(handle, nullptr);
    glfwSetCharModsCallback(handle, nullptr);
    glfwSetDropCallback(handle, nullptr);

    u32 device;
    {
        std::shared_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        device = static_window.owning_device;
    }
    hc::render::destroy_swapchain(handle, device);

    window->handle = nullptr;
}

namespace hc::window {
    void destroy(GLFWwindow *window) {
        if (window) {
            const char *name = glfwGetWindowTitle(window);
            if (name) {
                HC_INFO("Destroying window (handle: " << window << ')');
                std::unique_lock lock(window_mutex);
                window_map.erase(window);
                glfwDestroyWindow(window);
            } else {
                HC_WARN("Invalid window or GLFW context");
            }
        }
    }

    bool is_resizing(GLFWwindow *window) {
        std::shared_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(window);
        return static_window.resizing;
    }
}

void hc_set_window_position_callback(HCWindow *window, HCWindowPositionCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.position_callback = callback;
        if (callback) {
            HC_TRACE("Setting window position callback for window " << window->id);
            glfwSetWindowPosCallback(handle, [](GLFWwindow *glfw_window, int width, int height) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.position_callback(static_window.id, width, height);
            });
        } else {
            HC_TRACE("Unsetting window position callback for window " << window->id);
            glfwSetWindowPosCallback(handle, nullptr);
        }
    }
}

void hc_set_window_size_callback(HCWindow *window, HCWindowSizeCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.size_callback = callback;
        if (callback) {
            HC_TRACE("Setting window size callback for window " << window->id);
            glfwSetWindowSizeCallback(handle, [](GLFWwindow *glfw_window, int width, int height) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.size_callback(static_window.id, width, height);
            });
        } else {
            HC_TRACE("Unsetting window size callback for window " << window->id);
            glfwSetWindowSizeCallback(handle, nullptr);
        }
    }
}

void hc_set_window_close_callback(HCWindow *window, HCWindowCloseCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.close_callback = callback;
        if (callback) {
            HC_TRACE("Setting window close callback for window " << window->id);
            glfwSetWindowCloseCallback(handle, [](GLFWwindow *glfw_window) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.close_callback(static_window.id);
            });
        } else {
            HC_TRACE("Unsetting window close callback for window " << window->id);
            glfwSetWindowCloseCallback(handle, nullptr);
        }
    }
}

void hc_set_window_refresh_callback(HCWindow *window, HCWindowRefreshCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.refresh_callback = callback;
        if (callback) {
            HC_TRACE("Setting window refresh callback for window " << window->id);
            glfwSetWindowRefreshCallback(handle, [](GLFWwindow *glfw_window) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.refresh_callback(static_window.id);
            });
        } else {
            HC_TRACE("Unsetting window refresh callback for window " << window->id);
            glfwSetWindowRefreshCallback(handle, nullptr);
        }
    }
}

void hc_set_window_focus_callback(HCWindow *window, HCWindowFocusCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.focus_callback = callback;
        if (callback) {
            HC_TRACE("Setting window focus callback for window " << window->id);
            glfwSetWindowFocusCallback(handle, [](GLFWwindow *glfw_window, int focused) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.focus_callback(static_window.id, focused == GLFW_TRUE);
            });
        } else {
            HC_TRACE("Unsetting window focus callback for window " << window->id);
            glfwSetWindowFocusCallback(handle, nullptr);
        }
    }
}

void hc_set_window_minimize_callback(HCWindow *window, HCWindowMinimizeCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.minimize_callback = callback;
        if (callback) {
            HC_TRACE("Setting window minimize callback for window " << window->id);
            glfwSetWindowIconifyCallback(handle, [](GLFWwindow *glfw_window, int minimized) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.minimize_callback(static_window.id, minimized == GLFW_TRUE);
            });
        } else {
            HC_TRACE("Unsetting window minimize callback for window " << window->id);
            glfwSetWindowIconifyCallback(handle, nullptr);
        }
    }
}

void hc_set_window_maximize_callback(HCWindow *window, HCWindowMaximizeCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.maximize_callback = callback;
        if (callback) {
            HC_TRACE("Setting window maximize callback for window " << window->id);
            glfwSetWindowMaximizeCallback(handle, [](GLFWwindow *glfw_window, int maximized) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.maximize_callback(static_window.id, maximized == GLFW_TRUE);
            });
        } else {
            HC_TRACE("Unsetting window maximize callback for window " << window->id);
            glfwSetWindowMaximizeCallback(handle, nullptr);
        }
    }
}

void hc_set_window_framebuffer_callback(HCWindow *window, HCWindowFramebufferCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.framebuffer_callback = callback;
        if (callback) {
            HC_TRACE("Setting window framebuffer callback for window " << window->id);
            glfwSetFramebufferSizeCallback(handle, [](GLFWwindow *glfw_window, int width, int height) {
                // Unique lock because a variable is being set.
                std::unique_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                // Set resizing to true to keep the window's swapchain from recreating itself,
                // while glfwPollEvents is blocking.
                // This way, swapchains are only recreated at the end of glfwPollEvents, when the user stops resizing
                // the window.
                static_window.resizing = true;
                static_window.framebuffer_callback(static_window.id, width, height);
            });
        } else {
            HC_TRACE("Unsetting window framebuffer callback for window " << window->id);
            glfwSetFramebufferSizeCallback(handle, nullptr);
        }
    }
}

void hc_set_window_scale_callback(HCWindow *window, HCWindowScaleCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.scale_callback = callback;
        if (callback) {
            HC_TRACE("Setting window scale callback for window " << window->id);
            glfwSetWindowContentScaleCallback(handle, [](GLFWwindow *glfw_window, float x_scale, float y_scale) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.scale_callback(static_window.id, x_scale, y_scale);
            });
        } else {
            HC_TRACE("Unsetting window scale callback for window " << window->id);
            glfwSetWindowContentScaleCallback(handle, nullptr);
        }
    }
}

void hc_set_window_mouse_button_callback(HCWindow *window, HCWindowMouseButtonCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.mouse_button_callback = callback;
        if (callback) {
            HC_TRACE("Setting mouse button callback for window " << window->id);
            glfwSetMouseButtonCallback(handle, [](GLFWwindow *glfw_window, int button, int action, int mods) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.mouse_button_callback(static_window.id, hc::from_glfw_button(button),
                                                    hc::from_glfw_action(action), mods);
            });
        } else {
            HC_TRACE("Unsetting mouse button callback for window " << window->id);
            glfwSetMouseButtonCallback(handle, nullptr);
        }
    }
}

void hc_set_window_cursor_position_callback(HCWindow *window, HCWindowCursorPositionCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.cursor_position_callback = callback;
        if (callback) {
            HC_TRACE("Setting cursor position callback for window " << window->id);
            glfwSetCursorPosCallback(handle, [](GLFWwindow *glfw_window, double x, double y) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.cursor_position_callback(static_window.id, x, y);
            });
        } else {
            HC_TRACE("Unsetting cursor position callback for window " << window->id);
            glfwSetCursorPosCallback(handle, nullptr);
        }
    }
}

void hc_set_window_cursor_enter_callback(HCWindow *window, HCWindowCursorEnterCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.cursor_enter_callback = callback;
        if (callback) {
            HC_TRACE("Setting cursor enter callback for window " << window->id);
            glfwSetCursorEnterCallback(handle, [](GLFWwindow *glfw_window, int entered) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.cursor_enter_callback(static_window.id, entered == GLFW_TRUE);
            });
        } else {
            HC_TRACE("Unsetting cursor enter callback for window " << window->id);
            glfwSetCursorEnterCallback(handle, nullptr);
        }
    }
}

void hc_set_window_scroll_callback(HCWindow *window, HCWindowScrollCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.scroll_callback = callback;
        if (callback) {
            HC_TRACE("Setting scroll callback for window " << window->id);
            glfwSetScrollCallback(handle, [](GLFWwindow *glfw_window, double x_offset, double y_offset) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.scroll_callback(static_window.id, x_offset, y_offset);
            });
        } else {
            HC_TRACE("Unsetting scroll callback for window " << window->id);
            glfwSetScrollCallback(handle, nullptr);
        }
    }
}

void hc_set_window_key_callback(HCWindow *window, HCWindowKeyCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.key_callback = callback;
        if (callback) {
            HC_TRACE("Setting key callback for window " << window->id);
            glfwSetKeyCallback(handle, [](GLFWwindow *glfw_window, int key, int scan_code, int action, int mods) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.key_callback(static_window.id, hc::from_glfw_key(key), scan_code,
                                           hc::from_glfw_action(action), mods);
            });
        } else {
            HC_TRACE("Unsetting key callback for window " << window->id);
            glfwSetKeyCallback(handle, nullptr);
        }
    }
}

void hc_set_window_char_callback(HCWindow *window, HCWindowCharCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.char_callback = callback;
        if (callback) {
            HC_TRACE("Setting character callback for window " << window->id);
            glfwSetCharCallback(handle, [](GLFWwindow *glfw_window, unsigned int code_point) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.char_callback(static_window.id, code_point);
            });
        } else {
            HC_TRACE("Unsetting character callback for window " << window->id);
            glfwSetCharCallback(handle, nullptr);
        }
    }
}

void hc_set_window_char_mods_callback(HCWindow *window, HCWindowCharModsCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.char_mod_callback = callback;
        if (callback) {
            HC_TRACE("Setting character with mods callback for window " << window->id);
            glfwSetCharModsCallback(handle, [](GLFWwindow *glfw_window, unsigned int code_point, int mods) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.char_mod_callback(static_window.id, code_point, mods);
            });
        } else {
            HC_TRACE("Unsetting character with mods callback for window " << window->id);
            glfwSetCharModsCallback(handle, nullptr);
        }
    }
}

void hc_set_window_drop_callback(HCWindow *window, HCWindowDropCallback callback) {
    if (window->handle) {
        auto *handle = static_cast<GLFWwindow *>(window->handle);
        std::unique_lock lock(window_mutex);
        StaticWindow &static_window = window_map.at(handle);
        static_window.drop_callback = callback;
        if (callback) {
            HC_TRACE("Setting drop callback for window " << window->id);
            glfwSetDropCallback(handle, [](GLFWwindow *glfw_window, int path_count, const char *paths[]) {
                std::shared_lock lock(window_mutex);
                StaticWindow &static_window = window_map.at(glfw_window);
                static_window.drop_callback(static_window.id, path_count, paths);
            });
        } else {
            HC_TRACE("Unsetting drop callback for window " << window->id);
            glfwSetDropCallback(handle, nullptr);
        }
    }
}

#endif // HC_HEADLESS
