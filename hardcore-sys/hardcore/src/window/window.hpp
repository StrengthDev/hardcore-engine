#pragma once

#ifndef HC_HEADLESS

#include <core/window.h>

#include "handle.hpp"

#include <core/error.hpp>
#include <core/glfw.hpp>

#include <util/number.hpp>

#include <expected>

namespace hc::window {
    struct ContextParams {
        u64 window_id;
        bool raw_mouse_input_available;
    };

    class Window {
    public:
        static std::expected<Window, Error> create(
            ContextParams const& context_params,
            HCWindowParams const& window_params
        );

        ~Window();

        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        void disable() noexcept;

        [[nodiscard]] u64 id() const noexcept;
        [[nodiscard]] GLFWwindow const* handle() const noexcept;

        std::expected<void, Error> set_cursor_mode(HCCursorMode cursor_mode) noexcept;

        void set_position_callback(HCWindowPositionCallback callback) noexcept;
        void set_size_callback(HCWindowSizeCallback callback) noexcept;
        void set_close_callback(HCWindowCloseCallback callback) noexcept;
        void set_refresh_callback(HCWindowRefreshCallback callback) noexcept;
        void set_focus_callback(HCWindowFocusCallback callback) noexcept;
        void set_minimize_callback(HCWindowMinimizeCallback callback) noexcept;
        void set_maximize_callback(HCWindowMaximizeCallback callback) noexcept;
        void set_framebuffer_callback(HCWindowFramebufferCallback callback) noexcept;
        void set_scale_callback(HCWindowScaleCallback callback) noexcept;
        void set_mouse_button_callback(HCWindowMouseButtonCallback callback) noexcept;
        void set_cursor_position_callback(HCWindowCursorPositionCallback callback) noexcept;
        void set_cursor_enter_callback(HCWindowCursorEnterCallback callback) noexcept;
        void set_scroll_callback(HCWindowScrollCallback callback) noexcept;
        void set_key_callback(HCWindowKeyCallback callback) noexcept;
        void set_char_callback(HCWindowCharCallback callback) noexcept;
        void set_char_mods_callback(HCWindowCharModsCallback callback) noexcept;
        void set_drop_callback(HCWindowDropCallback callback) noexcept;

        void finish_resize();

    private:
        Window() = default;

        void resize(int width, int height);

        Handle glfw_handle;
        u64 window_id = std::numeric_limits<Sz>::max();
        u32 owning_device = std::numeric_limits<u32>::max();
        bool resizing = false;
        VkExtent2D extent = {};

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
}

#endif // HC_HEADLESS
