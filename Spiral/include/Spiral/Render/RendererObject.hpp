#pragma once

#include "Renderer.hpp"

#include "RenderCore.hpp"

#include "Device.hpp"
#include "ShaderLibrary.hpp"

#define MAX_DEVICES 8

namespace Spiral
{
	class RendererObject : public Renderer
	{
	public:
		RendererObject(ECProperties engineProps, ECProperties clientProps);
		~RendererObject();

		void m_presentFrame() override;
		void m_loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId) override;


	private:
		VkInstance instance;
		VkSurfaceKHR surface;
		Device availableDevices[MAX_DEVICES];
		uint32_t nAvailableDevices;
		uint32_t presentDeviceIndex;

		VkDebugUtilsMessengerEXT debugMessenger;
	};
}