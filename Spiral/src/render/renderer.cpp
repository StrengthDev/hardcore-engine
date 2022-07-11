#include <pch.hpp>

#include <spiral/render/renderer.hpp>
#include <spiral/render/renderer_internal.hpp>
#include <spiral/render/render_core.hpp>
#include <spiral/render/shader_library.hpp>

#include <spiral/core/static_client.hpp>
#include <spiral/core/window_internal.hpp>

namespace Spiral
{
	namespace renderer
	{
		const uint8_t max_devices = 8;

		VkInstance instance;
		VkSurfaceKHR surface;
		device available_devices[max_devices];
		uint32_t n_available_devices;
		uint32_t present_device_idx;

		VkDebugUtilsMessengerEXT debug_messenger;

		bool check_validation_layer_support()
		{
			uint32_t layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

			std::vector<VkLayerProperties> available_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

			for (const char* layer_name : validation_layers)
			{
				bool layer_found = false;

				for (const auto& layer_properties : available_layers)
				{
					if (strcmp(layer_name, layer_properties.layerName) == 0)
					{
						layer_found = true;
						break;
					}
				}

				if (!layer_found)
				{
					return false;
				}
			}

			return true;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
			VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
		{
			switch (message_severity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				LOGF_INTERNAL_TRACE("[VULKAN] {0}", callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				LOGF_INTERNAL_INFO("[VULKAN] {0}", callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				LOGF_INTERNAL_WARN("[VULKAN] {0}", callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				LOGF_INTERNAL_ERROR("[VULKAN] {0}", callback_data->pMessage);
				break;
			}
			return VK_FALSE;
		}

		VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger)
		{
			PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			if (func != nullptr)
			{
				return func(instance, create_info, allocator, debug_messenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (func != nullptr)
			{
				func(instance, debug_messenger, allocator);
			}
		}

		void init(program_id engine, program_id client)
		{
			if (enable_validation_layers && !check_validation_layer_support())
			{
				DEBUG_BREAK;
				shutdown();
				return;
			}
			else
			{
				uint32_t n_extensions = 0;
				vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, nullptr);
				std::vector<VkExtensionProperties> extensions(n_extensions);
				vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, extensions.data());
				LOG_INTERNAL_DEBUG("[VULKAN] Available extensions:");
				for (const auto& extension : extensions)
				{
					LOGF_INTERNAL_DEBUG("[VULKAN]  - {0}", extension.extensionName);
				}
			}

			VkResult result;
			uint32_t i;

			VkApplicationInfo app_info = {};	//has pNext for extension information
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = client.name;
			app_info.applicationVersion = VK_MAKE_API_VERSION(0, client.major, client.minor, client.patch); //TODO: variant?
			app_info.pEngineName = engine.name;
			app_info.engineVersion = VK_MAKE_API_VERSION(0, engine.major, engine.minor, engine.patch); //TODO: variant?
			app_info.apiVersion = VK_API_VERSION_1_1;
			uint32_t n_glfw_extensions = 0;
			const char** glfw_extensions;
			glfw_extensions = glfwGetRequiredInstanceExtensions(&n_glfw_extensions);	//gets required platform interface extensions
			std::vector<const char*> extensions(glfw_extensions, glfw_extensions + n_glfw_extensions);
			if (enable_validation_layers)
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app_info;
			instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			instance_info.ppEnabledExtensionNames = extensions.data();
			if (enable_validation_layers)
			{
				instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
				instance_info.ppEnabledLayerNames = validation_layers.data();
			}
			else
			{
				instance_info.enabledLayerCount = 0;
			}
			result = vkCreateInstance(&instance_info, nullptr, &instance);
			if (result != VK_SUCCESS)
			{
				DEBUG_BREAK;
				shutdown();
				return;
			}

			if (enable_validation_layers)
			{
				VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
				debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				debug_info.pfnUserCallback = debug_callback;
				debug_info.pUserData = nullptr; // Optional, passed to the callback function

				result = create_debug_utils_messenger_EXT(instance, &debug_info, nullptr, &debug_messenger);
				if (result != VK_SUCCESS)
				{
					DEBUG_BREAK;
					shutdown();
					return;
				}
			}

			GLFWwindow* window = window::get_handle();
			result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
			if (result != VK_SUCCESS)
			{
				DEBUG_BREAK;
				shutdown();
				return;
			}

			n_available_devices = 0;
			vkEnumeratePhysicalDevices(instance, &n_available_devices, nullptr);
			if (n_available_devices == 0)
			{
				DEBUG_BREAK;
				shutdown();
				return;
			}
			VkPhysicalDevice* devices = t_malloc<VkPhysicalDevice>(n_available_devices);
			vkEnumeratePhysicalDevices(instance, &n_available_devices, devices);
			for (i = 0; i < n_available_devices; i++)
			{
				new (&available_devices[i]) device(devices[i], surface);
			}
			free(devices);
		
			present_device_idx = 0; //TODO: select present device

			if (!available_devices[present_device_idx].create_swapchain())
			{
				DEBUG_BREAK;
				shutdown();
				return;
			}
			ShaderLibrary::init();
		}

		void terminate()
		{
			uint32_t i;
			for (i = 0; i < n_available_devices; i++)
			{
				available_devices[i].~device();
			}

			if (enable_validation_layers)
			{
				destroy_debug_utils_messenger_EXT(instance, debug_messenger, nullptr);
			}

			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);
			ShaderLibrary::terminate();
		}

		void tick()
		{
			available_devices[present_device_idx].draw();
		}

		device& get_device()
		{
			return available_devices[present_device_idx];
		}

		void loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId)
		{
			available_devices[present_device_idx].loadMesh(mesh, vertexShaderId, fragShaderId);
		}
	}
}
