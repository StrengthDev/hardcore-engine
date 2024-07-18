#include <pch.hpp>

#include <render/renderer.hpp>
#include <render/renderer_internal.hpp>
#include <render/render_core.hpp>

#include <core/static_client.hpp>
#include <core/window_internal.hpp>
#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	namespace renderer
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		std::array<device, 8> devices;
		std::uint32_t n_devices;
		std::uint32_t present_device_idx;

		VkDebugUtilsMessengerEXT debug_messenger;

		inline bool check_validation_layer_support()
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

		inline void list_extensions()
		{
			uint32_t n_extensions = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, nullptr);
			std::vector<VkExtensionProperties> extensions(n_extensions);
			vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, extensions.data());

#ifndef NDEBUG
			LOG_INTERNAL_INFO("[VULKAN] Available extensions:");
			for (const auto& extension : extensions)
			{
				LOGF_INTERNAL_INFO("[VULKAN]  - {0}", extension.extensionName);
			}
#endif // !NDEBUG

			//TODO: maybe add extensions to a list to check when eventual extensions are used
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
			VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
		{
			char type[] = "|---|";
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) type[1] = 'G';
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) type[2] = 'V';
			if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) type[3] = 'P';

			//Unsure if message_severity returns only 1 active bit
			if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				LOGF_INTERNAL_ERROR("[VULKAN] {0} {1}", type, callback_data->pMessage);
			}
			else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				LOGF_INTERNAL_WARN("[VULKAN] {0} {1}", type, callback_data->pMessage);
			}
			else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			{
				LOGF_INTERNAL_INFO("[VULKAN] {0} {1}", type, callback_data->pMessage);
			}
			else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			{
				LOGF_INTERNAL_TRACE("[VULKAN] {0} {1}", type, callback_data->pMessage);
			}

			return VK_FALSE;
		}

		inline VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger)
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

		inline void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (func != nullptr)
			{
				func(instance, debug_messenger, allocator);
			}
		}

		inline void create_instance(const program_id& engine, const program_id& client)
		{
			VkApplicationInfo app_info = {};	//has pNext for extension information
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = client.name;
			app_info.applicationVersion = VK_MAKE_API_VERSION(0, client.major, client.minor, client.patch);
			app_info.pEngineName = engine.name;
			app_info.engineVersion = VK_MAKE_API_VERSION(0, engine.major, engine.minor, engine.patch);
			app_info.apiVersion = VK_API_VERSION_1_1;
			uint32_t n_glfw_extensions = 0;
			const char** glfw_extensions;
			glfw_extensions = glfwGetRequiredInstanceExtensions(&n_glfw_extensions);	//gets required platform interface extensions
			std::vector<const char*> extensions(glfw_extensions, glfw_extensions + n_glfw_extensions);
			if constexpr (enable_validation_layers)
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app_info;
			instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			instance_info.ppEnabledExtensionNames = extensions.data();
			if constexpr (enable_validation_layers)
			{
				instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
				instance_info.ppEnabledLayerNames = validation_layers.data();
			}
			else
			{
				instance_info.enabledLayerCount = 0;
			}

			VK_CRASH_CHECK(vkCreateInstance(&instance_info, nullptr, &instance), "Failed to create Vulkan instance");
		}

		inline void init_devices()
		{
			n_devices = 0;
			vkEnumeratePhysicalDevices(instance, &n_devices, nullptr);
			if (n_devices == 0)
			{
				CRASH("No physical devices found");
			}
			INTERNAL_ASSERT(n_devices < devices.size(), "Too many devices found");
			VkPhysicalDevice* physical_devices = t_malloc<VkPhysicalDevice>(n_devices);
			vkEnumeratePhysicalDevices(instance, &n_devices, physical_devices);
			for (std::uint32_t i = 0; i < n_devices; i++)
			{
				devices[i].~device();
				new (&devices[i]) device(std::move(physical_devices[i]), surface);
			}
			std::free(physical_devices);
		
			present_device_idx = 0;
			std::size_t max_score = 0;
			for (std::uint32_t i = 0; i < n_devices; i++)
			{
				std::size_t score = devices[i].score();
				if (score > max_score)
				{
					present_device_idx = i;
					max_score = score;
				}
			}

			if (!devices[present_device_idx].create_swapchain())
			{
				CRASH("Failed to create window swapchain");
			}
		}

		void init(program_id engine, program_id client)
		{
			if constexpr (enable_validation_layers)
			{
				if (!check_validation_layer_support())
				{
					CRASH("No validation layer support");
				}
			}

			list_extensions();
			
			create_instance(engine, client);

			if constexpr (enable_validation_layers)
			{
				VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
				debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				debug_info.pfnUserCallback = debug_callback;
				debug_info.pUserData = nullptr; // Optional, passed to the callback function

				VK_CRASH_CHECK(create_debug_utils_messenger_EXT(instance, &debug_info, nullptr, &debug_messenger), 
					"Failed to create debug messenger");
			}

			GLFWwindow* window = window::get_handle();
			VK_CRASH_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create window surface");

			init_devices();
		}

		void terminate()
		{
			//Devices must be destroyed before the instance
			for (std::uint32_t i = 0; i < n_devices; i++)
			{
				devices[i].~device();
			}

			if (enable_validation_layers)
			{
				destroy_debug_utils_messenger_EXT(instance, debug_messenger, nullptr);
			}

			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);

			shader_library::clear();
		}

		void tick()
		{
			devices[present_device_idx].draw();
		}

		device& get_device()
		{
			return devices[present_device_idx];
		}
	}
}
