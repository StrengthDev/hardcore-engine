
#pragma once

#include "window.hpp"

#include <util/bank.hpp>
#include <util/concurrent_queue.hpp>

#include <expected>
#include <unordered_map>

namespace hc::window {
    class Context {
    public:
        Context() = default;

        Context(Context&&) = delete;
        Context(Context const&) = delete;

        Context& operator=(Context&&) = delete;
        Context& operator=(const Context&) = delete;

        static Context& instance();

        [[nodiscard]] std::expected<void, Error> init();

        void terminate();

        [[nodiscard]] std::expected<u64, Error> create_window(HCWindowParams const& window_params) noexcept;

        // Called from the device, which may be running in another thread, should be synchronized
        void destroy_window(GLFWwindow const* handle) noexcept;

        void yield_window(u64 id) noexcept;

        [[nodiscard]] Window& get_window(GLFWwindow const* handle);
        [[nodiscard]] Window const& get_window(GLFWwindow const* handle) const;

        [[nodiscard]] Window* find_window(u64 id);
        [[nodiscard]] Window const* find_window(u64 id) const;

        void poll_events();

    private :
        Bank<GLFWwindow const*> window_handles;
        std::unordered_map<GLFWwindow const*, Window> window_map;

        ConcurrentQueue<GLFWwindow const*> windows_to_destroy;

        bool raw_mouse_input_available = false;

        bool initialized = false;
    };
}
