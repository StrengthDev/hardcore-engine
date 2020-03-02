#include "pch.hpp"

#include "Device.hpp"

namespace Spiral
{
	Device::Device()
	{
		hasHandle = false;
	}

	void Device::init(VkPhysicalDevice physical, VkSurfaceKHR surface)
	{
		uint32_t i;
		physicalHandle = physical;
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
		for (i = 0; i < queueFamilyCount; i++) //TODO: Performance is better if both graphics and present queues are in the same family
		{
			if (queueFamilies[i].queueCount && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsIndex = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &presentSupport);
			if (queueFamilies[i].queueCount && presentSupport)
			{
				presentIndex = i;
			}

			if (graphicsIndex != -1 && presentIndex != -1)
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
				VkDeviceQueueCreateInfo info = {}; //TODO: might wanna change this to make more queues, etc
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
		free(queueFamilies);
		free(queueCreateInfos);

		swapChain = SwapChain();
	}

	void Device::terminate() //Note: when an object is destroyed, the member variables are only destroyed AFTER its destructor is called
	{
		if (hasHandle)
		{
			vkDestroyDevice(handle, nullptr);
		}
	}
}