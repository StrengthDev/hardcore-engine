#pragma once

#include "Renderer.hpp"

#include "RenderCore.hpp"

#include "Device.hpp"

#define MAX_DEVICES 8

namespace Spiral
{
	class RendererObject : public Renderer
	{
	public:
		RendererObject(ECProperties engineProps, ECProperties clientProps);
		~RendererObject();

		void presentFrame() override;


	private:
		VkInstance instance;
		VkSurfaceKHR surface;
		Device availableDevices[MAX_DEVICES];
		uint32_t nAvailableDevices;
		uint16_t presentDeviceIndex;

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}