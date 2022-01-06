#include "pch.hpp"

#include <Spiral/Render/Device.hpp>
#include <Spiral/Render/ShaderLibrary.hpp>

namespace Spiral
{
	void Device::init(VkPhysicalDevice physical, VkSurfaceKHR surface)
	{
		uint32_t i;
		currentFrame = 0;
		surfaceHandle = surface;
		physicalHandle = physical;
		hasHandle = false;
		hasSwapchain = false;
		vkGetPhysicalDeviceProperties(physicalHandle, &properties);
		vkGetPhysicalDeviceFeatures(physicalHandle, &features);

		SPRL_CORE_INFO("[RENDERER] Physical device found: {0}", properties.deviceName);

		graphicsIndex = -1;
		presentIndex = -1;
		computeIndex = -1;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);
		VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, queueFamilies);
		for (i = 0; i < queueFamilyCount; i++) //TODO: Performance is better if both graphics and present queues are in the same family, also should probably optimize queue selection
		{
			if (queueFamilies[i].queueCount && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsIndex = i;
			}

			if (queueFamilies[i].queueCount && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				computeIndex = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &presentSupport);
			if (queueFamilies[i].queueCount && presentSupport)
			{
				presentIndex = i;
			}

			if (graphicsIndex != -1 && presentIndex != -1 && computeIndex != -1)
			{
				break;
			}
		}
		//TODO: verify extension compatibility

		i = 0;
		std::set<int64_t> uniqueQueueFamilies = { graphicsIndex, presentIndex }; //TODO: no need to use a set, make an array with the max possible amount of different queue types and check if the one youre adding is already in the array, then malloc an array with the result
		VkDeviceQueueCreateInfo* queueCreateInfos = (VkDeviceQueueCreateInfo*)malloc(sizeof(VkDeviceQueueCreateInfo) * uniqueQueueFamilies.size());
		float queuePriority = 1.0f;
		for (int64_t index : uniqueQueueFamilies)
		{
			if (index != -1)
			{
				VkDeviceQueueCreateInfo info = {}; //TODO: make Queue struct, look at queue selection in DEV bookmarks
				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = (uint32_t)index;
				info.queueCount = 1;
				info.pQueuePriorities = &queuePriority;
				queueCreateInfos[i] = info;
				i++;
			}
		}
		
		VkPhysicalDeviceFeatures deviceFeatures = {}; //TODO: idk
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos;
		createInfo.queueCreateInfoCount = i;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		if (vkCreateDevice(physical, &createInfo, nullptr, &handle) == VK_SUCCESS) //This looks wrong and stupid
		{
			hasHandle = true;
			if (graphicsIndex != -1)
			{
				vkGetDeviceQueue(handle, (uint32_t)graphicsIndex, 0, &graphicsQueue);
			}
			if (presentIndex != -1)
			{
				vkGetDeviceQueue(handle, (uint32_t)presentIndex, 0, &presentQueue);
			}
			if (computeIndex != -1)
			{
				vkGetDeviceQueue(handle, (uint32_t)computeIndex, 0, &computeQueue);
			}
		}
		//SPRL_CORE_TRACE("Graphics family index: {0}", graphicsIndex);
		//SPRL_CORE_TRACE("Present family index: {0}", presentIndex);
		//SPRL_CORE_TRACE("Compute family index: {0}", computeIndex);
		free(queueFamilies);
		free(queueCreateInfos);

		//DeviceData data = { &physicalHandle, &handle, &graphicsQueue };
		//TODO: make a command pool construct to facilitate having one pool per thread per frame
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		if (vkCreateCommandPool(handle, &poolInfo, nullptr, &gCommandPool) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		Memory::init(&physicalHandle, &handle, &graphicsQueue, &gCommandPool);
	}

	void Device::terminate() //Note: when an object is destroyed, the member variables are only destroyed AFTER its destructor is called
	{
		size_t i;
		if (hasSwapchain)
		{
			if (gPipeline.valid)
			{
				gPipeline.terminate(handle, swapchain.nImages);
			}
			swapchain.terminate(handle);
			for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(handle, renderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(handle, imageAvailableSemaphores[i], nullptr);
				vkDestroyFence(handle, inFlightFences[i], nullptr);
			}
			vkDestroyCommandPool(handle, gCommandPool, nullptr);
		}
		Memory::terminate();
		if (hasHandle)
		{
			vkDestroyDevice(handle, nullptr);
		}
	}

	bool Device::createSwapchain()
	{
		swapchain.init(physicalHandle, handle, surfaceHandle, (uint32_t)graphicsIndex, (uint32_t)presentIndex);
		if (!swapchain.valid)
		{
			return false;
		}
		hasSwapchain = true;

		VkCommandPoolCreateInfo poolInfo = {}; //TODO: place in graphics pipeline, buffers in the same pool cannot be recorded concurrently
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = (uint32_t)graphicsIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Necessary to allow resetting command buffers
		if (vkCreateCommandPool(handle, &poolInfo, nullptr, &gCommandPool) != VK_SUCCESS)
		{
			return false;
		}

		size_t i;
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(handle, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(handle, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(handle, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				return false; //TODO: destroy objects that might have already been created
			}
		}

		return true;
	}

	bool Device::recreateSwapchain()
	{
		return true;
	}

	void Device::loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId)
	{
		Shader shaders[2];
		shaders[0] = ShaderLibrary::get(vertexShaderId);
		shaders[1] = ShaderLibrary::get(fragShaderId);
		gPipeline.init(handle, swapchain, gCommandPool, shaders, 2);
		gPipeline.loadMesh(mesh);
	}

	bool Device::drawFrame()
	{
		vkWaitForFences(handle, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(handle, swapchain.handle, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			//TODO: recreate swapchain
			return true; //Skip this frame
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			DEBUG_BREAK;
			return false;
		}
		gPipeline.draw(imageIndex);
		//TODO: update uniform buffer with imageIndex

		//TODO: move the queue submit to the pipeline
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &gPipeline.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

		vkResetFences(handle, 1, &inFlightFences[currentFrame]);
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) //TODO: porbably have to change this
		{
			DEBUG_BREAK;
			return false;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
		VkSwapchainKHR swapchains[] = { swapchain.handle };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			//TODO: recreate swapchain, add framebuffer resized(?)
		}
		else if (result != VK_SUCCESS)
		{
			DEBUG_BREAK;
			return false;
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		return true;
	}
}