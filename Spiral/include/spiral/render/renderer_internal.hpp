#pragma once

#include "renderer.hpp"

#include "render_core.hpp"

#include "device.hpp"
#include "shader_library.hpp"

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