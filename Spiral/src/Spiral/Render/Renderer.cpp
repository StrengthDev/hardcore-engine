#include "pch.hpp"

#include "RendererObject.hpp"

#include "Spiral/Core/Client.hpp"

namespace Spiral
{
	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		SPRL_CORE_DEBUG("[VULKAN] {0}", pCallbackData->pMessage);

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	RendererObject::RendererObject(ECProperties engineProps, ECProperties clientProps)
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			DEBUG_BREAK;
			Client::get().shutdown();
			return;
		}
		else
		{
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
			SPRL_CORE_INFO("[RENDERER] Available extensions:");
			for (const auto& extension : extensions)
			{
				SPRL_CORE_INFO("[RENDERER]  - {0}", extension.extensionName);
			}
		}

		VkResult result;
		uint32_t i;

		VkApplicationInfo appInfo = {};	//has pNext for extension information
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = clientProps.name;
		appInfo.applicationVersion = VK_MAKE_VERSION(clientProps.major, clientProps.minor, clientProps.patch);
		appInfo.pEngineName = engineProps.name;
		appInfo.engineVersion = VK_MAKE_VERSION(engineProps.major, engineProps.minor, engineProps.patch);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);	//gets required platform interface extensions
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
		{
			DEBUG_BREAK;
			Client::get().shutdown();
			return;
		}

		if (enableValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;
			createInfo.pUserData = nullptr; // Optional, passed to the callback function

			result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
			if (result != VK_SUCCESS)
			{
				DEBUG_BREAK;
				Client::get().shutdown();
				return;
			}
		}

		GLFWwindow *window = (GLFWwindow*)Client::get().getWindow().getInstance();
		result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		if (result != VK_SUCCESS)
		{
			DEBUG_BREAK;
			Client::get().shutdown();
			return;
		}

		nAvailableDevices = 0;
		vkEnumeratePhysicalDevices(instance, &nAvailableDevices, nullptr);
		if (nAvailableDevices == 0)
		{
			DEBUG_BREAK;
			Client::get().shutdown();
			return;
		}
		VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * nAvailableDevices);
		vkEnumeratePhysicalDevices(instance, &nAvailableDevices, devices);
		for (i = 0; i < nAvailableDevices; i++)
		{
			availableDevices[i].init(devices[i], surface);
		}
		free(devices);
		
		presentDeviceIndex = 0; //TODO: select present device

		if (!availableDevices[presentDeviceIndex].createSwapchain())
		{
			DEBUG_BREAK;
			Client::get().shutdown();
			return;
		}
	}

	RendererObject::~RendererObject()
	{
		uint32_t i;
		for (i = 0; i < nAvailableDevices; i++)
		{
			availableDevices[i].terminate();
		}

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void RendererObject::presentFrame()
	{
		if (availableDevices[presentDeviceIndex].drawFrame())
		{
			Client::get().shutdown();
		}
	}

	Renderer* Renderer::init(ECProperties engineProps, ECProperties clientProps)
	{
		return new RendererObject(engineProps, clientProps);
	}
}