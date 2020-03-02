#pragma once

#include "RenderCore.hpp"
#include "SwapChain.hpp"

namespace Spiral
{
	class Device //TODO: SLI/crossfire support, change to struct
	{
	public:
		Device();
		void init(VkPhysicalDevice physical, VkSurfaceKHR surface);
		void terminate();


		VkPhysicalDevice physicalHandle;
		VkDevice handle;
		bool hasHandle;

		SwapChain swapChain;

	private:
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		//Queue families
		int64_t graphicsIndex;
		VkQueue graphicsQueue;

		int64_t presentIndex;
		VkQueue presentQueue;

		int64_t computeIndex;
		VkQueue computeQueue;

	};
}