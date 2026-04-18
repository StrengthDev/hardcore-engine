
#include <pch.hpp>

#include "util.hpp"
#include "renderer.hpp"
#include "vars.hpp"
#include "vulkan.hpp"
#include "device/device.hpp"

#include <core/log.hpp>
#include <render/renderer.h>
#include <render/device.h>

#ifndef HC_HEADLESS
#include <core/glfw.hpp>
#endif // HC_HEADLESS

namespace hc::render {
    static auto constexpr VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
    static u32 constexpr VULKAN_API_VERSION = VK_API_VERSION_1_3;

    static u8 max_frames_in_flight_count = std::numeric_limits<u8>::max();
    static u8 frame_mod = std::numeric_limits<u8>::max();

    static vk::Instance global_instance;
    static vk::DebugUtilsMessenger debug_messenger;
    static HCVulkanDebugCallbackFn user_debug_callback = nullptr;
    static std::vector<device::Device> devices;

    static std::expected<std::vector<bool>, Error> layer_support(const std::vector<const char*>& layer_names) {
        u32 layer_count;
        VkResult result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query Vulkan instance layers: " << to_str(result));
            return Error(result);
        }

        std::vector<VkLayerProperties> available_layers(layer_count);
        result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query Vulkan instance layers: " << to_str(result));
            return Error(result);
        }

        for (auto& available_layer : available_layers) {
            HC_DEBUG("Layer available: " << available_layer.layerName);
        }

        std::vector<bool> found_layers(layer_names.size());
        std::ranges::fill(found_layers, false);
        u32 current = 0;
        for (auto& layer_name : layer_names) {
            for (auto& available_layer : available_layers) {
                if (strcmp(layer_name, available_layer.layerName) == 0) {
                    found_layers[current] = true;
                    break;
                }
            }

            current++;
        }

        return found_layers;
    }

    static std::expected<std::vector<bool>, Error> extension_support(
        const char* layer_name,
        const std::vector<const char*>& extension_names
    ) {
        u32 extension_count;
        VkResult result = vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query Vulkan instance extensions: " << to_str(result));
            return Error(result);
        }

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        result = vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, available_extensions.data());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query Vulkan instance extensions: " << to_str(result));
            return Error(result);
        }

        for (auto& available_extension : available_extensions) {
            HC_DEBUG("Extension available: " << available_extension.extensionName);
        }

        std::vector<bool> found_extensions(extension_names.size());
        std::ranges::fill(found_extensions, false);
        u32 current = 0;
        for (auto& extension_name : extension_names) {
            for (auto& available_extension : available_extensions) {
                if (strcmp(extension_name, available_extension.extensionName) == 0) {
                    found_extensions[current] = true;
                    break;
                }
            }

            current++;
        }

        return found_extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        [[maybe_unused]] void* user_data
    ) {
        char type[] = "[----]";
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            type[1] = 'G';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            type[2] = 'V';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            type[3] = 'P';
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
            type[4] = 'B';

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
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        [[maybe_unused]] void* user_data
    ) {
        int flags = 0;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
            flags |= HC_VK_GENERAL;
        }
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            flags |= HC_VK_VALIDATION;
        }
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            flags |= HC_VK_PERFORMANCE;
        }
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) {
            flags |= HC_VK_DEVICE_ADDRESS_BINDING;
        }

        HCLogKind kind = HCLogKind_Error;
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            kind = HCLogKind_Error;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            kind = HCLogKind_Warn;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            kind = HCLogKind_Info;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            kind = HCLogKind_Debug;
        }

        user_debug_callback(kind, flags, callback_data->pMessage);

        return VK_FALSE;
    }

    static std::expected<vk::Instance, Error> create_instance(
        const HCApplicationDescriptor& app,
        const std::vector<const char*>& layers
    ) {
        HC_INFO(
            "Using Vulkan " << HC_VULKAN_API_VERSION.major << '.' << HC_VULKAN_API_VERSION.minor << '.' << HC_VULKAN_API_VERSION.patch << " API"
        );
        HC_INFO(
            "Vulkan application: " << app.name << " v" << app.version.major << '.' << app.version.minor << '.' << app.
            version.patch
        );

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = app.name;
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, app.version.major, app.version.minor, app.version.patch);
        app_info.pEngineName = "Hardcore";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, HC_MAJOR, HC_MINOR, HC_MAJOR);
        app_info.apiVersion = VULKAN_API_VERSION;

        auto found_layers = layer_support(layers);
        if (!found_layers) {
            return found_layers.error();
        }

        bool layer_missing = false;
        for (u32 i = 0; i < layers.size(); i++) {
            HC_INFO("Using layer " << layers[i]);
            if (!(*found_layers)[i]) {
                HC_ERROR("Layer " << layers[i] << " is not supported");
                if (strcmp(layers[i], VALIDATION_LAYER_NAME) == 0) {
                    HC_ERROR("Make sure the Vulkan SDK is installed to be able to use validation layers");
                }
                layer_missing = true;
            }
        }
        if (layer_missing) {
            return Error(HCError_VulkanLayerNotFound);
        }

#ifdef HC_HEADLESS
        std::vector<const char*> extensions;
#else
        u32 glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
#endif // HC_HEADLESS

#ifdef HC_LOGGING
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // HC_LOGGING

        auto found_extensions = extension_support(nullptr, extensions);
        if (!found_extensions) {
            return found_extensions.error();
        }

        bool extension_missing = false;
        for (u32 i = 0; i < extensions.size(); i++) {
            HC_INFO("Using extension " << extensions[i]);
            if (!(*found_extensions)[i]) {
                HC_ERROR("Extension " << extensions[i] << " is not supported");
                extension_missing = true;
            }
        }
        if (extension_missing) {
            return Error(HCError_VulkanExtensionNotFound);
        }

        VkInstanceCreateInfo instance_info = {};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        instance_info.enabledExtensionCount = static_cast<u32>(extensions.size());
        instance_info.ppEnabledExtensionNames = extensions.data();
        instance_info.enabledLayerCount = static_cast<u32>(layers.size());
        instance_info.ppEnabledLayerNames = layers.data();

        return vk::Instance::create(&instance_info);
    }

    static std::expected<void, Error> init_devices(const std::vector<const char*>& layers) {
        u32 device_count = 0;
        VkResult result = vkEnumeratePhysicalDevices(global_instance, &device_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query physical devices: " << to_str(result));
            return Error(result);
        }
        if (!device_count) {
            HC_ERROR("No devices were found");
            return Error(HCError_NoDevices);
        }
        std::vector<VkPhysicalDevice> physical_handles(device_count);
        result = vkEnumeratePhysicalDevices(global_instance, &device_count, physical_handles.data());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query physical devices: " << to_str(result));
            return Error(result);
        }

        for (auto physical_handle : physical_handles) {
            auto device_result = device::Device::create(physical_handle, layers);
            if (device_result) {
                devices.push_back(*std::move(device_result));
            }
        }

        if (devices.empty()) {
            HC_ERROR("Failed to initialize all devices");
            return Error(HCError_NoDevices);
        }

        return {};
    }

    std::expected<void, Error> init(const HCApplicationDescriptor& app, const HCRenderParams& params) {
        if (params.max_frames_in_flight == 0) {
            HC_ERROR("Number of maximum frames in flight must be 1 or greater");
            return Error(HCError_InvalidParams);
        }
        max_frames_in_flight_count = params.max_frames_in_flight;

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to initialize Volk: " << to_str(result));
            return Error(result);
        }

        HC_INFO(
            "Vulkan header v" << HC_VULKAN_HEADERS_VERSION.major << '.' << HC_VULKAN_HEADERS_VERSION.minor << '.' << HC_VULKAN_HEADERS_VERSION.patch
        );
        HC_INFO("Volk header v" << HC_VOLK_HEADER_VERSION);

        std::vector<const char*> layers;

#ifdef HC_VULKAN_VALIDATION
        layers.push_back(VALIDATION_LAYER_NAME);
#endif // HC_VULKAN_VALIDATION

        auto instance_result = create_instance(app, layers);
        if (!instance_result) {
            return instance_result.error();
        }
        global_instance = *std::move(instance_result);

        volkLoadInstanceOnly(global_instance);

#ifdef HC_LOGGING
        VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        if (params.debug_callback) {
            user_debug_callback = params.debug_callback;
            debug_info.pfnUserCallback = custom_debug_callback;
        } else {
            debug_info.pfnUserCallback = default_debug_callback;
        }

        debug_info.pUserData = nullptr;

        auto debug_messenger_result = vk::DebugUtilsMessenger::create(global_instance, &debug_info);
        if (!debug_messenger_result) {
            return debug_messenger_result.error();
        }
        debug_messenger = *std::move(debug_messenger_result);
#endif // HC_LOGGING

        auto device_result = init_devices(layers);
        if (!device_result) {
            return device_result.error();
        }

        frame_mod = 0;

        return {};
    }

    void term() {
        devices.clear();

#ifdef HC_LOGGING
        debug_messenger.destroy(global_instance);
        user_debug_callback = nullptr;
#endif // HC_LOGGING

        global_instance.destroy();

        volkFinalize();

        max_frames_in_flight_count = std::numeric_limits<u8>::max();
    }

    u8 max_frames_in_flight() {
        return max_frames_in_flight_count;
    }

    u8 current_frame_mod() {
        return frame_mod;
    }

    VkInstance vk_instance() {
        return global_instance;
    }

    std::vector<device::Device>& device_list() noexcept {
        return devices;
    }

    std::expected<device::Device*, Error> device_at(u32 id) noexcept {
        if (devices.empty()) {
            HC_ERROR("No devices, the global instance may have not been initialised yet");
            return Error(HCError_NoDevices);
        }

        if (devices.size() <= id) {
            HC_ERROR("Device index out of bounds");
            return Error(HCError_NoSuchDevice);
        }

        return &devices[id];
    }
}

struct VersionBitfield {
    u32 patch : 12;
    u32 minor : 10;
    u32 major : 7;
    u32 variant : 3;
};

static constexpr HCVersion bitfield_to_version(u32 version_bitfield) {
    auto [patch, minor, major, variant] = std::bit_cast<VersionBitfield>(version_bitfield);
    return {.major = major, .minor = minor, .patch = patch,};
}

const HCVersion HC_VULKAN_API_VERSION = bitfield_to_version(hc::render::VULKAN_API_VERSION);
const HCVersion HC_VULKAN_HEADERS_VERSION = bitfield_to_version(VK_HEADER_VERSION_COMPLETE);
const u32 HC_VOLK_HEADER_VERSION = VOLK_HEADER_VERSION;

HCResult hc_render_tick() {
    std::expected<void, hc::Error> return_result = {};

    u8 next_mod = (hc::render::frame_mod + 1) % hc::render::max_frames_in_flight_count;

    for (auto& device : hc::render::devices) {
        auto tick_result = device.tick(hc::render::frame_mod, next_mod);
        if (!tick_result && return_result) {
            return_result = tick_result;
        }
    }

    hc::render::frame_mod = next_mod;

    if (!return_result) {
        return return_result.error();
    }

    return {.success = true};
}

HCResult hc_render_finish() {
    for (auto& device : hc::render::devices) {
        device.finish();
    }

    return {.success = true};
}

u32 hc_device_count() {
    return static_cast<u32>(hc::render::device_list().size());
}

const char* hc_device_name(u32 device) {
    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return nullptr;
    }

    return (*device_result)->name();
}
