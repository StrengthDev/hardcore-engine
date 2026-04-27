
#include <pch.hpp>

#include "handle.hpp"

#include <util/flow.hpp>

namespace hc::window {
    Handle::Handle(GLFWwindow* value) noexcept : value(value) {}

    Handle::Handle(Handle&& other) noexcept : value(std::exchange(other.value, nullptr)) {}

    Handle& Handle::operator=(Handle&& other) noexcept {
        HC_ASSERT(this->value == nullptr, "Window must be properly destroyed before being moved into");

        this->value = std::exchange(other.value, nullptr);

        return *this;
    }

    Handle::operator GLFWwindow*() const noexcept {
        return this->value;
    }
}
