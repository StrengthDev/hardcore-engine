#pragma once

#include "device/device.hpp"

#include <core/core.h>
#include <util/number.hpp>
#include <render/renderer.h>

namespace hc::render {
    enum class InstanceResult : u32 {
        Success = 0,
        VolkError,
        VulkanInstanceError,
        DebugCallbackError,
        DeviceError,
        NoDevicesFound,
        SurfaceFailure,
        Uninitialised,
        OutOfBounds,
        Unimplemented,
    };

    InstanceResult init(const HCApplicationDescriptor& app, const HCRenderParams& params);

    InstanceResult term();

    VkInstance instance();

    std::vector<device::Device>& device_list() noexcept;

    Result<device::Device*, InstanceResult> device_at(u32 id) noexcept;
}
