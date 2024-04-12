#include <pch.hpp>

#include <core/log.hpp>
#include <core/util.hpp>

#include "device.hpp"
#include "util.hpp"

namespace hc::render {
    std::optional<Device> Device::create(VkPhysicalDevice physical_handle, const std::vector<const char *> &layers) {
        Device device;
        vkGetPhysicalDeviceProperties(physical_handle, &device.properties);
        HC_INFO("Physical device found: " << device.properties.deviceName);
        vkGetPhysicalDeviceFeatures(physical_handle, &device.features);

        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_handle, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_handle, &queue_family_count, queue_families.data());

        std::vector<u32> graphics_queue_families;
        u32 compute_family = std::numeric_limits<u32>::max();
        u32 transfer_family = std::numeric_limits<u32>::max();
        device::Scheduler::select_queue_families(queue_families, graphics_queue_families, compute_family,
                                                 transfer_family);
        std::set<u32> unique_queue_families;

        if (graphics_queue_families.empty()) {
            HC_ERROR("No graphics queue families found");
            return std::nullopt;
        }
        unique_queue_families.insert(graphics_queue_families.begin(), graphics_queue_families.end());

        if (compute_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected compute queue family index: " << compute_family);
            unique_queue_families.insert(compute_family);
        } else {
            HC_WARN("No compute queue family found");
        }

        if (transfer_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected transfer queue family index: " << transfer_family);
        } else {
            HC_ERROR("No transfer queue family found");
            return std::nullopt;
        }
        unique_queue_families.insert(transfer_family);

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        queue_infos.reserve(unique_queue_families.size());
        float queue_priority = 1.0f;
        for (u32 index: unique_queue_families) {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = index;
            info.queueCount = 1;
            info.pQueuePriorities = &queue_priority;
            queue_infos.push_back(info);
        }

        VkPhysicalDeviceFeatures features = {};

        std::vector<const char *> extensions;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.pQueueCreateInfos = queue_infos.data();
        create_info.queueCreateInfoCount = static_cast<u32>(queue_infos.size());
        create_info.pEnabledFeatures = &features;
        create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = static_cast<u32>(layers.size());
        create_info.ppEnabledLayerNames = layers.data();

        VkDevice handle;
        VkResult res = vkCreateDevice(physical_handle, &create_info, nullptr, &handle);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to create Vulkan logical device: " << to_str(res));
            return std::nullopt;
        }

        volkLoadDeviceTable(&device.fn_table, handle);

        std::optional<device::Scheduler> scheduler = device::Scheduler::create(handle, device.fn_table, 1,
                                                                               std::move(queue_families),
                                                                               std::move(unique_queue_families),
                                                                               std::move(graphics_queue_families),
                                                                               compute_family, transfer_family);
        if (!scheduler) {
            HC_ERROR("Failed to create device scheduler");
            device.fn_table.vkDestroyDevice(handle, nullptr);
            return std::nullopt;
        }
        device.scheduler = std::move(*scheduler);

        device.physical_handle = physical_handle;
        device.handle = handle;

        return device;
    }

    void Device::destroy() {
        if (this->handle != VK_NULL_HANDLE) {
//            this->scheduler.destroy();
            this->fn_table.vkDestroyDevice(this->handle, nullptr);
            this->physical_handle = VK_NULL_HANDLE;
            this->handle = VK_NULL_HANDLE;
        }
    }

    Device::~Device() {
        this->destroy();
    }

    Device::Device(Device &&other) noexcept:
            physical_handle(std::exchange(other.physical_handle, VK_NULL_HANDLE)),
            properties(other.properties), features(other.features),
            scheduler(std::move(other.scheduler)),
            fn_table(std::exchange(other.fn_table, {})),
            handle(std::exchange(other.handle, VK_NULL_HANDLE)) {

    }

    Device &Device::operator=(Device &&other) noexcept {
        this->destroy();

        this->physical_handle = std::exchange(other.physical_handle, VK_NULL_HANDLE);
        this->properties = other.properties;
        this->features = other.features;
        this->scheduler = std::move(other.scheduler);
        this->fn_table = std::exchange(other.fn_table, {});
        this->handle = std::exchange(other.handle, VK_NULL_HANDLE);

        return *this;
    }
}
