#pragma once

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
    };

    InstanceResult init(const HCApplicationDescriptor &app);

    InstanceResult term();
}
