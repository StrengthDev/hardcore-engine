#include <pch.hpp>

#ifndef HC_HEADLESS
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#endif // HC_HEADLESS

#include <core/log.hpp>

#include "util.hpp"

#include "renderer.hpp"

#define VK_CHECK_RETURN(vk_fn_call)     \
{                                       \
    VkResult res = vk_fn_call;          \
    if (res != VK_SUCCESS) return res;  \
}(0)

namespace hc::render {
    static const char *VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

    static VkInstance global_instance = VK_NULL_HANDLE;
    static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    VkResult layer_support(const std::vector<const char *> &layer_names, std::vector<bool> &out_found_layers) {
        std::uint32_t layer_count;
        VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
        std::vector<VkLayerProperties> available_layers(layer_count);
        VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

        for (auto &available_layer: available_layers) {
            HC_DEBUG("Layer available: " << available_layer.layerName);
        }

        std::uint32_t current = 0;
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
        std::uint32_t extension_count;
        VK_CHECK_RETURN(vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, nullptr));
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        VK_CHECK_RETURN(
                vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, available_extensions.data()));

        for (auto &available_extension: available_extensions) {
            HC_DEBUG("Extension available: " << available_extension.extensionName);
        }

        std::uint32_t current = 0;
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
            [[maybe_unused]] void *user_data) {
        char type[] = "|---|";
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) type[1] = 'G';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) type[2] = 'V';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) type[3] = 'P';

        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            HC_ERROR("[VULKAN] " << type << ' ' << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            HC_WARN("[VULKAN] " << type << ' ' << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            HC_INFO("[VULKAN] " << type << ' ' << callback_data->pMessage);
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            HC_TRACE("[VULKAN] " << type << ' ' << callback_data->pMessage);
        }

        return VK_FALSE;
    }

    VkResult create_instance() {
        HC_INFO("Initialising Vulkan instance..");
        HC_INFO("Engine: Hardcore " << HC_MAJOR << '.' << HC_MINOR << '.' << HC_PATCH);
        HC_INFO("Application: ");

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "client.name";
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);
        app_info.pEngineName = "Hardcore";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, HC_MAJOR, HC_MINOR, HC_MAJOR);
        app_info.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char *> layers;

#ifdef HC_VULKAN_VALIDATION
        layers.push_back(VALIDATION_LAYER_NAME);
#endif // HC_VULKAN_VALIDATION

        std::vector<bool> found_layers(layers.size());
        std::fill(found_layers.begin(), found_layers.end(), false);
        VK_CHECK_RETURN(layer_support(layers, found_layers));
        bool support_success = true;
        for (std::uint32_t i = 0; i < layers.size(); i++) {
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
        std::uint32_t glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
#endif // HC_HEADLESS

#ifdef HC_LOGGING
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // HC_LOGGING

        std::vector<bool> found_extensions(extensions.size());
        std::fill(found_extensions.begin(), found_extensions.end(), false);
        VK_CHECK_RETURN(extension_support(nullptr, extensions, found_extensions));
        for (std::uint32_t i = 0; i < extensions.size(); i++) {
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
        instance_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        instance_info.ppEnabledExtensionNames = extensions.data();
        instance_info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
        instance_info.ppEnabledLayerNames = layers.data();

        return vkCreateInstance(&instance_info, nullptr, &global_instance);
    }

    InstanceResult init() {
        VkResult res = volkInitialize();
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize Volk: " << to_str(res));
            return InstanceResult::VolkError;
        }

        res = create_instance();
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize Vulkan instance: " << to_str(res));
            return InstanceResult::VolkError;
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
        debug_info.pfnUserCallback = debug_callback;
        debug_info.pUserData = nullptr;

        // vkCreateDebugUtilsMessengerEXT is loaded via Volk
        res = vkCreateDebugUtilsMessengerEXT(global_instance, &debug_info, nullptr, &debug_messenger);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to initialize debug messenger: " << to_str(res));
            return InstanceResult::VolkError;
        }
#endif // HC_LOGGING

        return InstanceResult::Success;
    }

    InstanceResult term() {
#ifdef HC_LOGGING
        vkDestroyDebugUtilsMessengerEXT(global_instance, debug_messenger, nullptr);
#endif // HC_LOGGING

        vkDestroyInstance(global_instance, nullptr);
        volkFinalize();
        return InstanceResult::Success;
    }
}
