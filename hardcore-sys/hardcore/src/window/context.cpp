
#include <pch.hpp>

#include "context.hpp"

#include "../core/error.hpp"
#include "../core/log.hpp"

namespace hc::window {
    static Context context_instance;

    static void glfw_error_callback(int error_code, const char* description) {
        HC_ERROR("GLFW: " << description << "(code " << error_code << ')');
    }

    Context& Context::instance() {
        return context_instance;
    }

    std::expected<void, Error> Context::init() {
        if (this->initialized) {
            HC_ERROR("GLFW context is already initialized");
            return Error(HCError_GLFWAlreadyInitialized);
        }

        if (!glfwInit()) {
            const char* description;
            glfwGetError(&description);
            HC_ERROR("Error initialising GLFW's context: " << description);
            return Error(HCError_GLFWInitFailed);
        }

        HC_INFO("GLFW v" << HC_GLFW_VERSION.major << '.' << HC_GLFW_VERSION.minor << '.' << HC_GLFW_VERSION.patch);

        glfwSetErrorCallback(glfw_error_callback);

        this->raw_mouse_input_available = glfwRawMouseMotionSupported();
        if (this->raw_mouse_input_available) {
            HC_INFO("Raw mouse input available");
        } else {
            HC_INFO("Raw mouse input unavailable");
        }

        this->initialized = true;

        return {};
    }

    void Context::terminate() {
        if (this->initialized) {
            // Destroy all windows before the context
            this->window_map.clear();

            glfwTerminate();

            this->initialized = false;
        }
    }

    std::expected<u64, Error> Context::create_window(HCWindowParams const& window_params) noexcept {
        u64 const id = this->window_handles.insert(nullptr);

        ContextParams const context_params = {
            .window_id = id,
            .raw_mouse_input_available = this->raw_mouse_input_available,
        };

        auto window_result = Window::create(context_params, window_params);
        if (!window_result) {
            return window_result.error();
        }

        GLFWwindow const* handle = window_result->handle();
        this->window_handles[id] = handle;
        this->window_map.emplace(handle, *std::move(window_result));

        return id;
    }

    void Context::destroy_window(GLFWwindow const* handle) noexcept {
        windows_to_destroy.push(handle);
    }

    void Context::yield_window(u64 id) noexcept {
        GLFWwindow const* handle = this->window_handles.erase(id);
        this->window_map.at(handle).disable();
    }

    Window& Context::get_window(GLFWwindow const* handle) {
        return this->window_map.at(handle);
    }

    Window const& Context::get_window(GLFWwindow const* handle) const {
        return this->window_map.at(handle);
    }

    Window* Context::find_window(u64 id) {
        auto handle = this->window_handles.find(id);
        if (!handle) {
            return nullptr;
        }

        if (auto it = this->window_map.find(*handle); it != this->window_map.end()) {
            return &it->second;
        }

        return nullptr;
    }

    Window const* Context::find_window(u64 id) const {
        auto handle = this->window_handles.find(id);
        if (!handle) {
            return nullptr;
        }

        if (auto it = this->window_map.find(*handle); it != this->window_map.end()) {
            return &it->second;
        }

        return nullptr;
    }

    void Context::poll_events() {
        glfwPollEvents();

        auto queue = this->windows_to_destroy.drain();
        while (!queue.empty()) {
            this->window_map.erase(queue.front());
            queue.pop();
        }

        for (auto& window : this->window_map | std::views::values) {
            window.finish_resize();
        }
    }
}
