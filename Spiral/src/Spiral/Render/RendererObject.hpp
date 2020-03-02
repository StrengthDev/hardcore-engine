#pragma once

#include "Renderer.hpp"

#include "RenderCore.hpp"

#include "Device.hpp"

#define MAX_DEVICES 32

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
		Device availableDevices[MAX_DEVICES];
		uint32_t nAvailableDevices;
		uint16_t presentDevice;

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}