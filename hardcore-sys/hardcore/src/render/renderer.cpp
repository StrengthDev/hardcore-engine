#include <pch.hpp>

#ifndef HC_HEADLESS
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#endif // HC_HEADLESS

#include <core/log.hpp>
#include <render/renderer.h>

#include "util.hpp"

#include "renderer.hpp"
#include "vars.hpp"
#include "device.hpp"

#define VK_CHECK_RETURN(vk_fn_call)     \
{                                       \
    VkResult res = vk_fn_call;          \
    if (res != VK_SUCCESS) return res;  \
}(0)

namespace hc::render {
    static const char *VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

    static u8 max_frames_in_flight_count = std::numeric_limits<u8>::max();
    static u8 frame_mod = std::numeric_limits<u8>::max();

    static VkInstance global_instance = VK_NULL_HANDLE;
    static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    static HCVulkanDebugCallbackFn user_debug_callback = nullptr;
    static std::vector<Device> devices;
    static u32 default_device_idx = std::numeric_limits<u32>::max();

    VkResult layer_support(const std::vector<const char *> &layer_names, std::vector<bool> &out_found_layers) {
        u32 layer_count;
        VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
        std::vector<VkLayerProperties> available_layers(layer_count);
        VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

        for (auto &available_layer: available_layers) {
            HC_DEBUG("Layer available: " << available_layer.layerName);
        }

        u32 current = 0;
        for (auto &layer_name: layer_names) {
            out_found_layers[current] = false;

            for (auto &available_layer: available_layers) {
                if (strcmp(layer_name, available_layer.layerName) == 0) {
                    out_found_layers[current] = true;
                    break;
                }
            }

            current++;
        }

        return VK_SUCCESS;
    }

    VkResult extension_support(const char *layer_name, const std::vector<const char *> &extension_names,
                               std::vector<bool> &out_found_extensions) {
        u32 extension_count;
        VK_CHECK_RETURN(vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, nullptr));
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        VK_CHECK_RETURN(
                vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, available_extensions.data()));

        for (auto &available_extension: available_extensions) {
            HC_DEBUG("Extension available: " << available_extension.extensionName);
        }

        u32 current = 0;
        for (auto &extension_name: extension_names) {
            out_found_extensions[current] = false;

            for (auto &available_extension: available_extensions) {
                if (strcmp(extension_name, available_extension.extensionName) == 0) {
                    out_found_extensions[current] = true;
                    break;
                }
            }

            current++;
        }

        return VK_SUCCESS;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
            [[maybe_unused]] void *user_data) {
        char type[] = "[----]";
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) type[1] = 'G';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) type[2] = 'V';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) type[3] = 'P';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) type[4] = 'B';

        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            HC_ERROR("Vulkan " << type << ": " << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            HC_WARN("Vulkan " << type << ": " << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            HC_INFO("Vulkan " << type << ": " << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            HC_TRACE("Vulkan " << type << ": " << callback_data->pMessage);
        }

        return VK_FALSE;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL custom_debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
            [[maybe_unused]] void *user_data) {
        int flags = 0;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            flags |= HC_VK_GENERAL;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            flags |= HC_VK_VALIDATION;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            flags |= HC_VK_PERFORMANCE;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
            flags |= HC_VK_DEVICE_ADDRESS_BINDING;

        HCLogKind kind = HCLogKind::Error;
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            kind = HCLogKind::Error;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            kind = HCLogKind::Warn;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            kind = HCLogKind::Info;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            kind = HCLogKind::Debug;
        }

        user_debug_callback(kind, flags, callback_data->pMessage);

        return VK_FALSE;
    }

    VkResult create_instance(const HCApplicationDescriptor &app, const std::vector<const char *> &layers) {
        HC_INFO("Initialising Vulkan instance..");
        HC_INFO("Engine: Hardcore v" << HC_MAJOR << '.' << HC_MINOR << '.' << HC_PATCH);
        HC_INFO("Application: " << app.name << " v" << app.major << '.' << app.minor << '.' << app.patch);

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = app.name;
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, app.major, app.minor, app.patch);
        app_info.pEngineName = "Hardcore";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, HC_MAJOR, HC_MINOR, HC_MAJOR);
        app_info.apiVersion = VK_API_VERSION_1_3;

        std::vector<bool> found_layers(layers.size());
        std::fill(found_layers.begin(), found_layers.end(), false);
        VK_CHECK_RETURN(layer_support(layers, found_layers));
        bool support_success = true;
        for (u32 i = 0; i < layers.size(); i++) {
            HC_INFO("Using layer " << layers[i]);
            if (!found_layers[i]) {
                HC_ERROR("Layer " << layers[i] << " is not supported");
                if (strcmp(layers[i], VALIDATION_LAYER_NAME) == 0) {
                    HC_ERROR("Make sure the Vulkan SDK is installed to be able to use validation layers");
                }
                support_success = false;
            }
        }
        if (!support_success) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

#ifdef HC_HEADLESS
        std::vector<const char *> extensions;
#else
        u32 glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
#endif // HC_HEADLESS

#ifdef HC_LOGGING
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // HC_LOGGING

        std::vector<bool> found_extensions(extensions.size());
        std::fill(found_extensions.begin(), found_extensions.end(), false);
        VK_CHECK_RETURN(extension_support(nullptr, extensions, found_extensions));
        for (u32 i = 0; i < extensions.size(); i++) {
            HC_INFO("Using extension " << extensions[i]);
            if (!found_extensions[i]) {
                HC_ERROR("Extension " << extensions[i] << " is not supported");
                support_success = false;
            }
        }
        if (!support_success) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        VkInstanceCreateInfo instance_info = {};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        instance_info.enabledExtensionCount = static_cast<u32>(extensions.size());
        instance_info.ppEnabledExtensionNames = extensions.data();
        instance_info.enabledLayerCount = static_cast<u32>(layers.size());
        instance_info.ppEnabledLayerNames = layers.data();

        return vkCreateInstance(&instance_info, nullptr, &global_instance);
    }


    InstanceResult init_devices(const std::vector<const char *> &layers) {
        u32 device_count = 0;
        VkResult res = vkEnumeratePhysicalDevices(global_instance, &device_count, nullptr);
        if (res != VK_SUCCESS) {
            return InstanceResult::DeviceError;
        }
        if (!device_count) {
            return InstanceResult::NoDevicesFound;
        }
        std::vector<VkPhysicalDevice> physical_handles(device_count);
        res = vkEnumeratePhysicalDevices(global_instance, &device_count, physical_handles.data());
        if (res != VK_SUCCESS) {
            return InstanceResult::DeviceError;
        }

        for (auto physical_handle: physical_handles) {
            std::optional<Device> device = Device::create(physical_handle, layers);
            if (device) {
                devices.push_back(std::move(*device));
            }
        }

        if (devices.empty()) {
            return InstanceResult::NoDevicesFound;
        }

        return InstanceResult::Success;
    }

    InstanceResult init(const HCApplicationDescriptor &app, const HCRenderParams &params) {
        max_frames_in_flight_count = params.max_frames_in_flight;

        VkResult res = volkInitialize();
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize Volk: " << to_str(res));
            return InstanceResult::VolkError;
        }

        std::vector<const char *> layers;

#ifdef HC_VULKAN_VALIDATION
        layers.push_back(VALIDATION_LAYER_NAME);
#endif // HC_VULKAN_VALIDATION

        res = create_instance(app, layers);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize Vulkan instance: " << to_str(res));
            return InstanceResult::VulkanInstanceError;
        }
        volkLoadInstanceOnly(global_instance);

#ifdef HC_LOGGING
        VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_info.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        if (params.debug_callback) {
            user_debug_callback = params.debug_callback;
            debug_info.pfnUserCallback = custom_debug_callback;
        } else {
            debug_info.pfnUserCallback = default_debug_callback;
        }

        debug_info.pUserData = nullptr;


        // vkCreateDebugUtilsMessengerEXT is loaded via Volk
        res = vkCreateDebugUtilsMessengerEXT(global_instance, &debug_info, nullptr, &debug_messenger);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize debug messenger: " << to_str(res));
            return InstanceResult::DebugCallbackError;
        }
#endif // HC_LOGGING

        InstanceResult device_res = init_devices(layers);
        if (device_res != InstanceResult::Success) {
            switch (device_res) {
                case InstanceResult::DeviceError: HC_ERROR("Failed to initialize Vulkan devices");
                    break;
                case InstanceResult::NoDevicesFound: HC_ERROR("Did not find any devices");
                    break;
                default: HC_ERROR("Unknown error while initialising Vulkan devices");
            }

            return device_res;
        }

        // TODO select most powerful device
        default_device_idx = 0;

        frame_mod = 0;

        return InstanceResult::Success;
    }

    InstanceResult term() {
        devices.clear();

#ifdef HC_LOGGING
        if (debug_messenger != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(global_instance, debug_messenger, nullptr);
            debug_messenger = VK_NULL_HANDLE;
            user_debug_callback = nullptr;
        }
#endif // HC_LOGGING

        if (global_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(global_instance, nullptr);
            global_instance = VK_NULL_HANDLE;
        }

        volkFinalize();

        max_frames_in_flight_count = std::numeric_limits<u8>::max();
        default_device_idx = std::numeric_limits<u32>::max();

        return InstanceResult::Success;
    }

    InstanceResult create_swapchain(GLFWwindow *window, u32 device_idx) {
        if (device_idx == std::numeric_limits<u32>::max()) {
            device_idx = default_device_idx;
        }

        HC_DEBUG("Creating new swapchain on device " << devices[device_idx].name() << " (id: " << device_idx << ')');
        DeviceResult res = devices[device_idx].create_swapchain(global_instance, window);
        switch (res) {
            case DeviceResult::Success:
                return InstanceResult::Success;
            case DeviceResult::SurfaceFailure:
                return InstanceResult::SurfaceFailure;
            default:
                return InstanceResult::Unimplemented;
        }
    }

    void destroy_swapchain(GLFWwindow *window, u32 device_idx) {
        // The default device may have changed meanwhile
        HC_ASSERT(device_idx != std::numeric_limits<u32>::max(), "Cannot assume default device");
        devices[device_idx].destroy_swapchain(global_instance, window, frame_mod);
    }

    u8 max_frames_in_flight() {
        return max_frames_in_flight_count;
    }

    VkInstance instance() {
        return global_instance;
    }

    u32 default_device() {
        return default_device_idx;
    }
}

int hc_render_tick() {
    u8 current_mod = hc::render::frame_mod;

    for (auto &device: hc::render::devices) {
        device.tick(current_mod);
    }

    current_mod++;
    hc::render::frame_mod = current_mod < hc::render::max_frames_in_flight() ? current_mod : 0;

    return 0;
}

int hc_render_finish() {
    for (u8 i = 0; i < hc::render::max_frames_in_flight_count; ++i) {
        int res = hc_render_tick();
        if (res)
            return res;
    }

    return 0;
}
