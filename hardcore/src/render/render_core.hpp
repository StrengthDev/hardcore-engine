#pragma once

#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <core/core.hpp>

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

#define VK_CRASH_CHECK(func, fail_message) { VkResult result = (func); if (result != VK_SUCCESS) { CRASH(fail_message, result); } }

const uint8_t max_frames_in_flight = 2;