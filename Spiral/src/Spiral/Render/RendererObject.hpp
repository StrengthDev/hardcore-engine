#pragma once

#include "Renderer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Spiral
{
	class RendererObject : public Renderer
	{
	public:
		RendererObject(ECProperties engineProps, ECProperties clientProps);
		~RendererObject();


	private:
		VkInstance instance;
		VkSurfaceKHR surface;
		VkPhysicalDevice* availableDevices;
		uint32_t nAvailableDevices;

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}