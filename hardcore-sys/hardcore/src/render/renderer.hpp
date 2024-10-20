#pragma once

#include <vector>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/core.h>
#include <util/number.hpp>
#include <render/renderer.h>

#include "device.hpp"

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

	InstanceResult init(const HCApplicationDescriptor &app, const HCRenderParams &params);

	InstanceResult term();

	VkInstance instance();

	std::vector<Device> &device_list() noexcept;

	Result<Device *, InstanceResult> device_at(u32 id) noexcept;
}
