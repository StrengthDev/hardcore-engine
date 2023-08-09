#pragma once

#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <core/core.hpp>
#include <debug/log_internal.hpp>

#define VK_CRASH_CHECK(func, fail_message) 						\
{ 																\
	VkResult result = (func); 									\
	if (result != VK_SUCCESS) 									\
	{															\
		LOG_INTERNAL_CRIT("Critical VULKAN error: " << result);	\
		CRASH(fail_message, result);							\
	}															\
}

namespace ENGINE_NAMESPACE
{
	const std::array<const char*, 1> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::array<const char*, 1> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enable_validation_layers = false;
#else
	const bool enable_validation_layers = true;
#endif

	const u8 max_frames_in_flight = 2;
}

