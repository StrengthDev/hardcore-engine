#include <pch.hpp>

#ifndef HC_HEADLESS

#include "window.hpp"
#include "error.hpp"
#include "log.hpp"

#include <core/window.h>
#include <render/renderer.hpp>
#include <render/util.hpp>
#include <util/number.hpp>

namespace hc::window {
    /**
     * @brief Static window callback function pointers.
     */
    struct StaticWindow {
        Sz id = std::numeric_limits<Sz>::max();
        u32 owning_device = std::numeric_limits<u32>::max();
        bool resizing = false;
        VkExtent2D extent;
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
     * @brief The list of windows that will be destroyed in the next call to `hc_poll_events`.
     */
    static std::vector<GLFWwindow*> windows_to_destroy;

    /**
     * @brief The mutex used to lock access to `window_map` and `windows_to_destroy`.
     */
    static std::shared_mutex window_mutex;

    /**
     * @brief A map that takes a GLFWwindow pointer as a key, and stores the respective window callbacks.
     *
     * This is needed because GLFW window callbacks do not accept user data and only provide the GLFWwindow pointer within
     * the callback, so this is used to access each window's individual callbacks.
     */
    static std::unordered_map<GLFWwindow*, StaticWindow> window_map;

    static bool raw_mouse_input_available = false;

    static void glfw_error_callback(int error_code, const char* description) {
        HC_ERROR("GLFW: " << description << "(code " << error_code << ')');
    }

    static inline void system_framebuffer_callback(StaticWindow& static_window, int width, int height) {
        // Set resizing to true to keep the window's swapchain from recreating itself, while glfwPollEvents is blocking.
        // This way, swapchains are only recreated at the end of glfwPollEvents, when the user stops
        // resizing the window.
        static_window.resizing = true;

        static_window.extent.width = static_cast<u32>(width);
        static_window.extent.height = static_cast<u32>(height);
    }

    std::expected<void, Error> init_context() {
        if (!glfwInit()) {
            const char* description;
            glfwGetError(&description);
            HC_ERROR("Error initialising GLFW's context: " << description);
            return Error(HCError_GLFWInitFailed);
        }

        HC_INFO("GLFW v" << HC_GLFW_VERSION.major << '.' << HC_GLFW_VERSION.minor << '.' << HC_GLFW_VERSION.patch);

        glfwSetErrorCallback(glfw_error_callback);

        raw_mouse_input_available = glfwRawMouseMotionSupported();
        if (raw_mouse_input_available) {
            HC_INFO("Raw mouse input available");
        } else {
            HC_INFO("Raw mouse input unavailable");
        }

        return {};
    }

    void terminate_context() {
        std::unique_lock lock(window_mutex);

        for (GLFWwindow* window : window_map | std::views::keys) {
            glfwDestroyWindow(window);
        }

        windows_to_destroy.clear();
        window_map.clear();

        glfwTerminate();
    }

    void destroy(GLFWwindow* window) {
        std::unique_lock lock(window_mutex);
        windows_to_destroy.push_back(window);
    }

    bool is_resizing(GLFWwindow* window) {
        std::shared_lock lock(window_mutex);
        StaticWindow& static_window = window_map.at(window);
        return static_window.resizing;
    }

    VkExtent2D extent(GLFWwindow* window) {
        std::shared_lock lock(window_mutex);
        StaticWindow& static_window = window_map.at(window);
        return static_window.extent;
    }

    static inline HCMouseButton from_glfw_button(int button) {
        switch (button) {
        case GLFW_MOUSE_BUTTON_1:
            return HCMouseButton_Button1;
        case GLFW_MOUSE_BUTTON_2:
            return HCMouseButton_Button2;
        case GLFW_MOUSE_BUTTON_3:
            return HCMouseButton_Button3;
        case GLFW_MOUSE_BUTTON_4:
            return HCMouseButton_Button4;
        case GLFW_MOUSE_BUTTON_5:
            return HCMouseButton_Button5;
        case GLFW_MOUSE_BUTTON_6:
            return HCMouseButton_Button6;
        case GLFW_MOUSE_BUTTON_7:
            return HCMouseButton_Button7;
        case GLFW_MOUSE_BUTTON_8:
            return HCMouseButton_Button8;
        default: HC_UNREACHABLE("All possible values must be handled");
        }
    }

    static inline HCButtonAction from_glfw_action(int action) {
        switch (action) {
        case GLFW_RELEASE:
            return HCButtonAction_Release;
        case GLFW_PRESS:
            return HCButtonAction_Press;
        case GLFW_REPEAT:
            return HCButtonAction_Repeat;
        default: HC_UNREACHABLE("All possible values must be handled");
        }
    }

    static inline HCKeyboardKey from_glfw_key(int key) {
        switch (key) {
        case GLFW_KEY_SPACE:
            return HCKeyboardKey_Space;
        case GLFW_KEY_APOSTROPHE:
            return HCKeyboardKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return HCKeyboardKey_Comma;
        case GLFW_KEY_MINUS:
            return HCKeyboardKey_Minus;
        case GLFW_KEY_PERIOD:
            return HCKeyboardKey_Period;
        case GLFW_KEY_SLASH:
            return HCKeyboardKey_Slash;
        case GLFW_KEY_0:
            return HCKeyboardKey_Num0;
        case GLFW_KEY_1:
            return HCKeyboardKey_Num1;
        case GLFW_KEY_2:
            return HCKeyboardKey_Num2;
        case GLFW_KEY_3:
            return HCKeyboardKey_Num3;
        case GLFW_KEY_4:
            return HCKeyboardKey_Num4;
        case GLFW_KEY_5:
            return HCKeyboardKey_Num5;
        case GLFW_KEY_6:
            return HCKeyboardKey_Num6;
        case GLFW_KEY_7:
            return HCKeyboardKey_Num7;
        case GLFW_KEY_8:
            return HCKeyboardKey_Num8;
        case GLFW_KEY_9:
            return HCKeyboardKey_Num9;
        case GLFW_KEY_SEMICOLON:
            return HCKeyboardKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return HCKeyboardKey_Equal;
        case GLFW_KEY_A:
            return HCKeyboardKey_A;
        case GLFW_KEY_B:
            return HCKeyboardKey_B;
        case GLFW_KEY_C:
            return HCKeyboardKey_C;
        case GLFW_KEY_D:
            return HCKeyboardKey_D;
        case GLFW_KEY_E:
            return HCKeyboardKey_E;
        case GLFW_KEY_F:
            return HCKeyboardKey_F;
        case GLFW_KEY_G:
            return HCKeyboardKey_G;
        case GLFW_KEY_H:
            return HCKeyboardKey_H;
        case GLFW_KEY_I:
            return HCKeyboardKey_I;
        case GLFW_KEY_J:
            return HCKeyboardKey_J;
        case GLFW_KEY_K:
            return HCKeyboardKey_K;
        case GLFW_KEY_L:
            return HCKeyboardKey_L;
        case GLFW_KEY_M:
            return HCKeyboardKey_M;
        case GLFW_KEY_N:
            return HCKeyboardKey_N;
        case GLFW_KEY_O:
            return HCKeyboardKey_O;
        case GLFW_KEY_P:
            return HCKeyboardKey_P;
        case GLFW_KEY_Q:
            return HCKeyboardKey_Q;
        case GLFW_KEY_R:
            return HCKeyboardKey_R;
        case GLFW_KEY_S:
            return HCKeyboardKey_S;
        case GLFW_KEY_T:
            return HCKeyboardKey_T;
        case GLFW_KEY_U:
            return HCKeyboardKey_U;
        case GLFW_KEY_V:
            return HCKeyboardKey_V;
        case GLFW_KEY_W:
            return HCKeyboardKey_W;
        case GLFW_KEY_X:
            return HCKeyboardKey_X;
        case GLFW_KEY_Y:
            return HCKeyboardKey_Y;
        case GLFW_KEY_Z:
            return HCKeyboardKey_Z;
        case GLFW_KEY_LEFT_BRACKET:
            return HCKeyboardKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return HCKeyboardKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return HCKeyboardKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return HCKeyboardKey_GraveAccent;
        case GLFW_KEY_WORLD_1:
            return HCKeyboardKey_World1;
        case GLFW_KEY_WORLD_2:
            return HCKeyboardKey_World2;
        case GLFW_KEY_ESCAPE:
            return HCKeyboardKey_Escape;
        case GLFW_KEY_ENTER:
            return HCKeyboardKey_Enter;
        case GLFW_KEY_TAB:
            return HCKeyboardKey_Tab;
        case GLFW_KEY_BACKSPACE:
            return HCKeyboardKey_Backspace;
        case GLFW_KEY_INSERT:
            return HCKeyboardKey_Insert;
        case GLFW_KEY_DELETE:
            return HCKeyboardKey_Delete;
        case GLFW_KEY_RIGHT:
            return HCKeyboardKey_Right;
        case GLFW_KEY_LEFT:
            return HCKeyboardKey_Left;
        case GLFW_KEY_DOWN:
            return HCKeyboardKey_Down;
        case GLFW_KEY_UP:
            return HCKeyboardKey_Up;
        case GLFW_KEY_PAGE_UP:
            return HCKeyboardKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return HCKeyboardKey_PageDown;
        case GLFW_KEY_HOME:
            return HCKeyboardKey_Home;
        case GLFW_KEY_END:
            return HCKeyboardKey_End;
        case GLFW_KEY_CAPS_LOCK:
            return HCKeyboardKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return HCKeyboardKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return HCKeyboardKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return HCKeyboardKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return HCKeyboardKey_Pause;
        case GLFW_KEY_F1:
            return HCKeyboardKey_F1;
        case GLFW_KEY_F2:
            return HCKeyboardKey_F2;
        case GLFW_KEY_F3:
            return HCKeyboardKey_F3;
        case GLFW_KEY_F4:
            return HCKeyboardKey_F4;
        case GLFW_KEY_F5:
            return HCKeyboardKey_F5;
        case GLFW_KEY_F6:
            return HCKeyboardKey_F6;
        case GLFW_KEY_F7:
            return HCKeyboardKey_F7;
        case GLFW_KEY_F8:
            return HCKeyboardKey_F8;
        case GLFW_KEY_F9:
            return HCKeyboardKey_F9;
        case GLFW_KEY_F10:
            return HCKeyboardKey_F10;
        case GLFW_KEY_F11:
            return HCKeyboardKey_F11;
        case GLFW_KEY_F12:
            return HCKeyboardKey_F12;
        case GLFW_KEY_F13:
            return HCKeyboardKey_F13;
        case GLFW_KEY_F14:
            return HCKeyboardKey_F14;
        case GLFW_KEY_F15:
            return HCKeyboardKey_F15;
        case GLFW_KEY_F16:
            return HCKeyboardKey_F16;
        case GLFW_KEY_F17:
            return HCKeyboardKey_F17;
        case GLFW_KEY_F18:
            return HCKeyboardKey_F18;
        case GLFW_KEY_F19:
            return HCKeyboardKey_F19;
        case GLFW_KEY_F20:
            return HCKeyboardKey_F20;
        case GLFW_KEY_F21:
            return HCKeyboardKey_F21;
        case GLFW_KEY_F22:
            return HCKeyboardKey_F22;
        case GLFW_KEY_F23:
            return HCKeyboardKey_F23;
        case GLFW_KEY_F24:
            return HCKeyboardKey_F24;
        case GLFW_KEY_F25:
            return HCKeyboardKey_F25;
        case GLFW_KEY_KP_0:
            return HCKeyboardKey_Numpad0;
        case GLFW_KEY_KP_1:
            return HCKeyboardKey_Numpad1;
        case GLFW_KEY_KP_2:
            return HCKeyboardKey_Numpad2;
        case GLFW_KEY_KP_3:
            return HCKeyboardKey_Numpad3;
        case GLFW_KEY_KP_4:
            return HCKeyboardKey_Numpad4;
        case GLFW_KEY_KP_5:
            return HCKeyboardKey_Numpad5;
        case GLFW_KEY_KP_6:
            return HCKeyboardKey_Numpad6;
        case GLFW_KEY_KP_7:
            return HCKeyboardKey_Numpad7;
        case GLFW_KEY_KP_8:
            return HCKeyboardKey_Numpad8;
        case GLFW_KEY_KP_9:
            return HCKeyboardKey_Numpad9;
        case GLFW_KEY_KP_DECIMAL:
            return HCKeyboardKey_NumpadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return HCKeyboardKey_NumpadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return HCKeyboardKey_NumpadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return HCKeyboardKey_NumpadSubtract;
        case GLFW_KEY_KP_ADD:
            return HCKeyboardKey_NumpadAdd;
        case GLFW_KEY_KP_ENTER:
            return HCKeyboardKey_NumpadEnter;
        case GLFW_KEY_KP_EQUAL:
            return HCKeyboardKey_NumpadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return HCKeyboardKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return HCKeyboardKey_LeftControl;
        case GLFW_KEY_LEFT_ALT:
            return HCKeyboardKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return HCKeyboardKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return HCKeyboardKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return HCKeyboardKey_RightControl;
        case GLFW_KEY_RIGHT_ALT:
            return HCKeyboardKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return HCKeyboardKey_RightSuper;
        case GLFW_KEY_MENU:
            return HCKeyboardKey_Menu;
        default: HC_UNREACHABLE("All possible values must be handled");
        }
    }

    static inline HCDeviceEvent from_glfw_event(int event) {
        switch (event) {
        case GLFW_CONNECTED:
            return HCDeviceEvent_Connected;
        case GLFW_DISCONNECTED:
            return HCDeviceEvent_Disconnected;
        default: HC_UNREACHABLE("All possible values must be handled");
        }
    }
}

const HCVersion HC_GLFW_VERSION = {GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION};

void hc_poll_events() {
    glfwPollEvents();

    // Unsure if this needed, as callbacks should only be called from within glfwPollEvents.
    std::unique_lock lock(hc::window::window_mutex);

    for (GLFWwindow* window : hc::window::windows_to_destroy) {
        HC_INFO("Destroying window " << hc::window::window_map.at(window).id << " (handle: " << window << ')');
        hc::window::window_map.erase(window);
        glfwDestroyWindow(window);
    }
    hc::window::windows_to_destroy.clear();

    for (auto& window : hc::window::window_map | std::views::values) {
        window.resizing = false;
    }
}

HCResult hc_new_window(HCWindow* window, HCWindowParams params) {
    if (!window) {
        return {.error = HCError_InvalidParams, .success = false};
    }

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

    auto device_result = hc::render::device_at(params.device);
    if (!device_result) {
        return device_result.error();
    }

    GLFWwindow* window_handle = glfwCreateWindow(
        static_cast<int>(params.width),
        static_cast<int>(params.height),
        params.name,
        nullptr,
        nullptr
    );

    if (!window_handle) {
        const char* description;
        glfwGetError(&description);
        HC_ERROR("Failed to create window: " << description);
        return {.error = HCError_GLFWWindowCreationFailure, .success = false};
    }

    glfwSetInputMode(window_handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    if (hc::window::raw_mouse_input_available) {
        glfwSetInputMode(window_handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetFramebufferSizeCallback(
        window_handle,
        [](GLFWwindow* glfw_window, int width, int height) {
            // Unique lock because a variable is being set.
            std::unique_lock lock(hc::window::window_mutex);
            hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
            hc::window::system_framebuffer_callback(static_window, width, height);
        }
    );

    int width, height;
    glfwGetFramebufferSize(window_handle, &width, &height);
    VkExtent2D extent{static_cast<u32>(width), static_cast<u32>(height)};

    VkInstance instance = hc::render::vk_instance();
    ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE> surface;
    VkResult surface_result = glfwCreateWindowSurface(instance, window_handle, nullptr, &surface.get());
    if (surface_result != VK_SUCCESS) {
        HC_ERROR("Failed to create window surface: " << hc::render::to_str(surface_result));
        glfwDestroyWindow(window_handle);
        return hc::Error(surface_result);
    }

    auto swapchain_result = (*device_result)->create_swapchain(window_handle, std::move(surface), extent);
    if (!swapchain_result) {
        HC_ERROR("Failed to create swapchain");
        glfwDestroyWindow(window_handle);
        return swapchain_result.error();
    }

    // Find first available ID
    std::unique_lock lock(hc::window::window_mutex);
    std::vector<Sz> ids;
    ids.reserve(hc::window::window_map.size());
    for (auto& static_window : hc::window::window_map | std::views::values) {
        ids.push_back(static_window.id);
    }
    std::ranges::sort(ids);
    Sz id = 0;
    for (auto item : ids) {
        if (id == item) {
            id++;
        } else {
            break;
        }
    }

    hc::window::StaticWindow static_window;
    static_window.id = id;
    static_window.owning_device = params.device;
    static_window.extent = extent;
    hc::window::window_map.emplace(window_handle, static_window);

    HC_INFO("Created new window with id " << id);

    window->handle = window_handle;
    window->id = id;

    return {.success = true};
}

void hc_destroy_window(HCWindow* window) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    HC_INFO("Window " << window->id << " marked for destruction (handle: " << window->handle << ')');
    auto* handle = static_cast<GLFWwindow*>(window->handle);

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
        std::shared_lock lock(hc::window::window_mutex);
        hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
        device = static_window.owning_device;
    }
    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        HC_UNREACHABLE("Windows should always refer to a valid device");
    }

    (*device_result)->destroy_swapchain(handle);

    window->handle = nullptr;
}

void hc_set_window_cursor_mode(HCWindow* window, HCCursorMode cursor_mode) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);

    u32 mode;
    switch (cursor_mode) {
    case HCCursorMode_Normal:
        HC_TRACE("Setting cursor mode normal for window " << window->id);
        mode = GLFW_CURSOR_NORMAL;
        break;
    case HCCursorMode_Hidden:
        HC_TRACE("Setting cursor mode hidden for window " << window->id);
        mode = GLFW_CURSOR_HIDDEN;
        break;
    case HCCursorMode_Captured:
        HC_TRACE("Setting cursor mode captured for window " << window->id);
        mode = GLFW_CURSOR_CAPTURED;
        break;
    case HCCursorMode_Disabled:
        HC_TRACE("Setting cursor mode disabled for window " << window->id);
        mode = GLFW_CURSOR_DISABLED;
        break;
    default:
        HC_ERROR("Invalid cursor mode value.");
        return;
    }

    glfwSetInputMode(handle, GLFW_CURSOR, mode);
}

void hc_set_window_position_callback(HCWindow* window, HCWindowPositionCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.position_callback = callback;
    if (callback) {
        HC_TRACE("Setting window position callback for window " << window->id);
        glfwSetWindowPosCallback(
            handle,
            [](GLFWwindow* glfw_window, int width, int height) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.position_callback(static_window.id, width, height);
            }
        );
    } else {
        HC_TRACE("Unsetting window position callback for window " << window->id);
        glfwSetWindowPosCallback(handle, nullptr);
    }
}

void hc_set_window_size_callback(HCWindow* window, HCWindowSizeCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.size_callback = callback;
    if (callback) {
        HC_TRACE("Setting window size callback for window " << window->id);
        glfwSetWindowSizeCallback(
            handle,
            [](GLFWwindow* glfw_window, int width, int height) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.size_callback(static_window.id, width, height);
            }
        );
    } else {
        HC_TRACE("Unsetting window size callback for window " << window->id);
        glfwSetWindowSizeCallback(handle, nullptr);
    }
}

void hc_set_window_close_callback(HCWindow* window, HCWindowCloseCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.close_callback = callback;
    if (callback) {
        HC_TRACE("Setting window close callback for window " << window->id);
        glfwSetWindowCloseCallback(
            handle,
            [](GLFWwindow* glfw_window) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.close_callback(static_window.id);
            }
        );
    } else {
        HC_TRACE("Unsetting window close callback for window " << window->id);
        glfwSetWindowCloseCallback(handle, nullptr);
    }
}

void hc_set_window_refresh_callback(HCWindow* window, HCWindowRefreshCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.refresh_callback = callback;
    if (callback) {
        HC_TRACE("Setting window refresh callback for window " << window->id);
        glfwSetWindowRefreshCallback(
            handle,
            [](GLFWwindow* glfw_window) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.refresh_callback(static_window.id);
            }
        );
    } else {
        HC_TRACE("Unsetting window refresh callback for window " << window->id);
        glfwSetWindowRefreshCallback(handle, nullptr);
    }
}

void hc_set_window_focus_callback(HCWindow* window, HCWindowFocusCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.focus_callback = callback;
    if (callback) {
        HC_TRACE("Setting window focus callback for window " << window->id);
        glfwSetWindowFocusCallback(
            handle,
            [](GLFWwindow* glfw_window, int focused) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.focus_callback(static_window.id, focused == GLFW_TRUE);
            }
        );
    } else {
        HC_TRACE("Unsetting window focus callback for window " << window->id);
        glfwSetWindowFocusCallback(handle, nullptr);
    }
}

void hc_set_window_minimize_callback(HCWindow* window, HCWindowMinimizeCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.minimize_callback = callback;
    if (callback) {
        HC_TRACE("Setting window minimize callback for window " << window->id);
        glfwSetWindowIconifyCallback(
            handle,
            [](GLFWwindow* glfw_window, int minimized) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.minimize_callback(static_window.id, minimized == GLFW_TRUE);
            }
        );
    } else {
        HC_TRACE("Unsetting window minimize callback for window " << window->id);
        glfwSetWindowIconifyCallback(handle, nullptr);
    }
}

void hc_set_window_maximize_callback(HCWindow* window, HCWindowMaximizeCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.maximize_callback = callback;
    if (callback) {
        HC_TRACE("Setting window maximize callback for window " << window->id);
        glfwSetWindowMaximizeCallback(
            handle,
            [](GLFWwindow* glfw_window, int maximized) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.maximize_callback(static_window.id, maximized == GLFW_TRUE);
            }
        );
    } else {
        HC_TRACE("Unsetting window maximize callback for window " << window->id);
        glfwSetWindowMaximizeCallback(handle, nullptr);
    }
}

void hc_set_window_framebuffer_callback(HCWindow* window, HCWindowFramebufferCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.framebuffer_callback = callback;
    if (callback) {
        HC_TRACE("Setting window framebuffer callback for window " << window->id);
        glfwSetFramebufferSizeCallback(
            handle,
            [](GLFWwindow* glfw_window, int width, int height) {
                // Unique lock because a variable is being set.
                std::unique_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                hc::window::system_framebuffer_callback(static_window, width, height);

                static_window.framebuffer_callback(static_window.id, width, height);
            }
        );
    } else {
        HC_TRACE("Unsetting window framebuffer callback for window " << window->id);
        glfwSetFramebufferSizeCallback(
            handle,
            [](GLFWwindow* glfw_window, int width, int height) {
                // Unique lock because a variable is being set.
                std::unique_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                hc::window::system_framebuffer_callback(static_window, width, height);
            }
        );
    }
}

void hc_set_window_scale_callback(HCWindow* window, HCWindowScaleCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.scale_callback = callback;
    if (callback) {
        HC_TRACE("Setting window scale callback for window " << window->id);
        glfwSetWindowContentScaleCallback(
            handle,
            [](GLFWwindow* glfw_window, float x_scale, float y_scale) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.scale_callback(static_window.id, x_scale, y_scale);
            }
        );
    } else {
        HC_TRACE("Unsetting window scale callback for window " << window->id);
        glfwSetWindowContentScaleCallback(handle, nullptr);
    }
}

void hc_set_window_mouse_button_callback(HCWindow* window, HCWindowMouseButtonCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.mouse_button_callback = callback;
    if (callback) {
        HC_TRACE("Setting mouse button callback for window " << window->id);
        glfwSetMouseButtonCallback(
            handle,
            [](GLFWwindow* glfw_window, int button, int action, int mods) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.mouse_button_callback(
                    static_window.id,
                    hc::window::from_glfw_button(button),
                    hc::window::from_glfw_action(action),
                    mods
                );
            }
        );
    } else {
        HC_TRACE("Unsetting mouse button callback for window " << window->id);
        glfwSetMouseButtonCallback(handle, nullptr);
    }
}

void hc_set_window_cursor_position_callback(HCWindow* window, HCWindowCursorPositionCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }
    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.cursor_position_callback = callback;
    if (callback) {
        HC_TRACE("Setting cursor position callback for window " << window->id);
        glfwSetCursorPosCallback(
            handle,
            [](GLFWwindow* glfw_window, double x, double y) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.cursor_position_callback(static_window.id, x, y);
            }
        );
    } else {
        HC_TRACE("Unsetting cursor position callback for window " << window->id);
        glfwSetCursorPosCallback(handle, nullptr);
    }
}

void hc_set_window_cursor_enter_callback(HCWindow* window, HCWindowCursorEnterCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.cursor_enter_callback = callback;
    if (callback) {
        HC_TRACE("Setting cursor enter callback for window " << window->id);
        glfwSetCursorEnterCallback(
            handle,
            [](GLFWwindow* glfw_window, int entered) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.cursor_enter_callback(static_window.id, entered == GLFW_TRUE);
            }
        );
    } else {
        HC_TRACE("Unsetting cursor enter callback for window " << window->id);
        glfwSetCursorEnterCallback(handle, nullptr);
    }
}

void hc_set_window_scroll_callback(HCWindow* window, HCWindowScrollCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.scroll_callback = callback;
    if (callback) {
        HC_TRACE("Setting scroll callback for window " << window->id);
        glfwSetScrollCallback(
            handle,
            [](GLFWwindow* glfw_window, double x_offset, double y_offset) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.scroll_callback(static_window.id, x_offset, y_offset);
            }
        );
    } else {
        HC_TRACE("Unsetting scroll callback for window " << window->id);
        glfwSetScrollCallback(handle, nullptr);
    }
}

void hc_set_window_key_callback(HCWindow* window, HCWindowKeyCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.key_callback = callback;
    if (callback) {
        HC_TRACE("Setting key callback for window " << window->id);
        glfwSetKeyCallback(
            handle,
            [](GLFWwindow* glfw_window, int key, int scan_code, int action, int mods) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.key_callback(
                    static_window.id,
                    hc::window::from_glfw_key(key),
                    scan_code,
                    hc::window::from_glfw_action(action),
                    mods
                );
            }
        );
    } else {
        HC_TRACE("Unsetting key callback for window " << window->id);
        glfwSetKeyCallback(handle, nullptr);
    }
}

void hc_set_window_char_callback(HCWindow* window, HCWindowCharCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.char_callback = callback;
    if (callback) {
        HC_TRACE("Setting character callback for window " << window->id);
        glfwSetCharCallback(
            handle,
            [](GLFWwindow* glfw_window, unsigned int code_point) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.char_callback(static_window.id, code_point);
            }
        );
    } else {
        HC_TRACE("Unsetting character callback for window " << window->id);
        glfwSetCharCallback(handle, nullptr);
    }
}

void hc_set_window_char_mods_callback(HCWindow* window, HCWindowCharModsCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.char_mod_callback = callback;
    if (callback) {
        HC_TRACE("Setting character with mods callback for window " << window->id);
        glfwSetCharModsCallback(
            handle,
            [](GLFWwindow* glfw_window, unsigned int code_point, int mods) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.char_mod_callback(static_window.id, code_point, mods);
            }
        );
    } else {
        HC_TRACE("Unsetting character with mods callback for window " << window->id);
        glfwSetCharModsCallback(handle, nullptr);
    }
}

void hc_set_window_drop_callback(HCWindow* window, HCWindowDropCallback callback) {
    if (!window || !window->handle) {
        HC_WARN("Null window pointer or window handle");
        return;
    }

    auto* handle = static_cast<GLFWwindow*>(window->handle);
    std::unique_lock lock(hc::window::window_mutex);

    if (!hc::window::window_map.contains(handle)) {
        HC_WARN("No such window: " << handle);
        return;
    }

    hc::window::StaticWindow& static_window = hc::window::window_map.at(handle);
    static_window.drop_callback = callback;
    if (callback) {
        HC_TRACE("Setting drop callback for window " << window->id);
        glfwSetDropCallback(
            handle,
            [](GLFWwindow* glfw_window, int path_count, const char* paths[]) {
                std::shared_lock lock(hc::window::window_mutex);
                hc::window::StaticWindow& static_window = hc::window::window_map.at(glfw_window);
                static_window.drop_callback(static_window.id, path_count, paths);
            }
        );
    } else {
        HC_TRACE("Unsetting drop callback for window " << window->id);
        glfwSetDropCallback(handle, nullptr);
    }
}

#endif // HC_HEADLESS
