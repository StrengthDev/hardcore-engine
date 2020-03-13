#pragma once

#include "RenderCore.hpp"
#include "Swapchain.hpp"

namespace Spiral
{
	struct Device //TODO: SLI/crossfire support
	{
		VkSurfaceKHR surfaceHandle;
		VkPhysicalDevice physicalHandle;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkDevice handle;
		bool hasHandle;
		uint32_t score; //TODO: calculate score

		//Queue families
		int64_t graphicsIndex;
		VkQueue graphicsQueue;

		int64_t presentIndex;
		VkQueue presentQueue;

		int64_t computeIndex;
		VkQueue computeQueue;

		//Rendering pipeline
		Swapchain swapchain;
		bool hasSwapchain;

		VkCommandBuffer* commandBuffers;

		size_t currentFrame;
		VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
		VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
		VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];



		void init(VkPhysicalDevice physical, VkSurfaceKHR surface);
		void terminate();

		bool createSwapchain();
		bool recreateSwapchain();
		bool drawFrame();
	};
}