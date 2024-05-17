#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/core.h>
#include <core/util.hpp>

namespace hc::render {
    enum class InstanceResult : u32 {
        Success = 0,
        VolkError,
        VulkanInstanceError,
        DebugCallbackError,
        DeviceError,
        NoDevicesFound,
        SurfaceFailure,
        Unimplemented,
        // TODO: add more errors, depending on what failed exactly, create Result type
    };

    InstanceResult init(const HCApplicationDescriptor &app, const HCRenderParams &params);

    InstanceResult term();

    InstanceResult create_swapchain(GLFWwindow *window, u32 device_idx);

    void destroy_swapchain(GLFWwindow *window, u32 device_idx);

    u32 default_device();
}
