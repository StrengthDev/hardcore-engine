#include <pch.hpp>

#include <core/log.hpp>
#include <core/window.hpp>
#include <util/flow.hpp>
#include <render/vars.hpp>

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
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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

        device.graph = device::Graph::create(device.scheduler.graphics_queue_family(),
                                             device.scheduler.compute_queue_family(),
                                             device.scheduler.transfer_queue_family());

        device.physical_handle = physical_handle;
        device.handle = handle;
        device.cleanup_queues = std::vector<std::vector<device::DestructionMark>>(max_frames_in_flight());

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
            swapchains(std::move(other.swapchains)),
            cleanup_queues(std::move(other.cleanup_queues)),
            handle(std::exchange(other.handle, VK_NULL_HANDLE)),
            fn_table(std::exchange(other.fn_table, {})) {

    }

    Device &Device::operator=(Device &&other) noexcept {
        this->destroy();

        this->physical_handle = std::exchange(other.physical_handle, VK_NULL_HANDLE);
        this->properties = other.properties;
        this->features = other.features;
        this->scheduler = std::move(other.scheduler);
        this->swapchains = std::move(other.swapchains);
        this->cleanup_queues = std::move(other.cleanup_queues);
        this->handle = std::exchange(other.handle, VK_NULL_HANDLE);
        this->fn_table = std::exchange(other.fn_table, {});

        return *this;
    }

    void Device::tick(u8 frame_mod) {
        // Destroy objects marked for destruction that are no longer being used
        auto &cleanup_queue = this->cleanup_queues[frame_mod];
        for (auto &mark: cleanup_queue) {
            std::visit(device::DestructionMarkHandler{
                    [this](device::WindowDestructionMark window_mark) {
                        auto node = this->swapchains.extract(window_mark.window);
                        HC_ASSERT(!node.empty(), "A swapchain matching the mark's window should exist");
                        node.mapped().destroy(window_mark.instance, this->fn_table, this->handle);
                        window::destroy(window_mark.window);
                    },
                    [this](device::SwapchainDestructionMark swapchain_mark) {
                        HC_ASSERT(this->swapchains.contains(swapchain_mark.window),
                                  "A swapchain matching the mark's window should exist");
                        this->swapchains.at(swapchain_mark.window).destroy_old(this->fn_table, this->handle);
                    },
            }, mark);
        }
        cleanup_queue.clear();

        device::GraphResult graph_res = this->graph.compile();
        HC_ASSERT(graph_res == device::GraphResult::Success, "Graph compilation should always succeed");

        // Rendering and presentation
//        for (auto &[window, swapchain]: this->swapchains) {
//            auto res = swapchain.acquire_image(this->fn_table, this->handle, window, frame_mod);
//            if (!res)
//                continue;
//
//            u32 image_index = res.ok();
//        }

//        this->graph.record();
    }

    const char *Device::name() const noexcept {
        return this->properties.deviceName;
    }

    std::optional<VkSurfaceCapabilities2KHR>
    surface_capabilities(VkPhysicalDevice physical_handle, const VkSurfaceKHR &surface) {
        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                .pNext = nullptr,
                .surface = surface,
        };

        VkSurfaceCapabilities2KHR capabilities = {
                .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
                .pNext = nullptr,
                .surfaceCapabilities = {},
        };
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_handle, &surface_info, &capabilities);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface capabilities: " << to_str(res));
            return std::nullopt;
        }

        return capabilities;
    }

    std::vector<VkSurfaceFormat2KHR> surface_formats(VkPhysicalDevice physical_handle, const VkSurfaceKHR &surface) {
        u32 format_count = 0;

        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                .pNext = nullptr,
                .surface = surface,
        };

        VkResult res = vkGetPhysicalDeviceSurfaceFormats2KHR(physical_handle, &surface_info, &format_count,
                                                             nullptr);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface formats: " << to_str(res));
            return {};
        }
        std::vector<VkSurfaceFormat2KHR> formats(format_count);
        for (auto &format: formats) {
            format.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }
        res = vkGetPhysicalDeviceSurfaceFormats2KHR(physical_handle, &surface_info, &format_count,
                                                    formats.data());
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface formats: " << to_str(res));
            return {};
        }

        return formats;
    }

    std::vector<VkPresentModeKHR> surface_present_modes(VkPhysicalDevice physical_handle, const VkSurfaceKHR &surface) {
        u32 mode_count = 0;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_handle, surface, &mode_count, nullptr);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display modes: " << to_str(res));
            return {};
        }
        std::vector<VkPresentModeKHR> present_modes(mode_count);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_handle, surface, &mode_count,
                                                        present_modes.data());
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display modes: " << to_str(res));
            return {};
        }

        // The specification requires that this present mode is supported, if the surface is supported
        if (present_modes.empty())
            present_modes.push_back(VK_PRESENT_MODE_FIFO_KHR);

        return present_modes;
    }

    DeviceResult Device::create_swapchain(VkInstance instance, GLFWwindow *window) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to create window surface: " << to_str(res));
            return DeviceResult::SurfaceFailure;
        }

        auto graphics_present_queues = this->scheduler.present_support(this->physical_handle, surface);

        if (graphics_present_queues.first == std::numeric_limits<u32>::max()
            || graphics_present_queues.second == std::numeric_limits<u32>::max()) {
            HC_ERROR("Presentation not supported for swapchain surface");
            vkDestroySurfaceKHR(instance, surface, nullptr);
            return DeviceResult::SwapchainFailure;
        }

        auto capabilities = surface_capabilities(this->physical_handle, surface);
        if (!capabilities) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            return DeviceResult::VkFailure;
        }

        std::vector<VkSurfaceFormat2KHR> formats = surface_formats(this->physical_handle, surface);
        if (formats.empty()) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            return DeviceResult::SurfaceFailure;
        }

        std::vector<VkPresentModeKHR> present_modes = surface_present_modes(this->physical_handle, surface);
        if (present_modes.empty()) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            return DeviceResult::SurfaceFailure;
        }

        device::SurfaceInfo surface_info = {
                .capabilities = *capabilities,
                .available_formats = std::move(formats),
                .available_present_modes = std::move(present_modes),
        };

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        device::SwapchainParams params = {
                .graphics_queue = graphics_present_queues.first,
                .present_queue = graphics_present_queues.second,
                .extent = VkExtent2D{.width = static_cast<u32>(width), .height = static_cast<u32>(height)},
                .preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
                .preferred_format = VkSurfaceFormatKHR{
                        .format = VK_FORMAT_B8G8R8A8_UNORM,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                },
        };

        auto swapchain = device::Swapchain::create(this->fn_table, this->handle, std::move(surface),
                                                   std::move(surface_info), std::move(params));
        if (swapchain) {
            this->swapchains.emplace(window, std::move(swapchain).ok());
            return DeviceResult::Success;
        } else {
            return DeviceResult::SwapchainFailure;
        }
    }

    void Device::destroy_swapchain(VkInstance instance, GLFWwindow *window, u8 frame_mod) {
        device::WindowDestructionMark mark = {
                .instance = instance,
                .window = window
        };
        this->cleanup_queues[frame_mod].emplace_back(mark);
    }

    Result<HCBuffer, DeviceResult> Device::new_buffer(HCBufferKind kind, resource::Descriptor &&descriptor, u64 count,
                                                      bool writable) {
        HC_ASSERT(kind != HCBufferKind::Index, "`new_index_buffer` should be used to create index buffers");
        HC_ASSERT(count, "Must have something to allocate");
        return Result<HCBuffer, DeviceResult>();
    }

    Result<HCBuffer, DeviceResult> Device::new_index_buffer(HCPrimitive index_type, u64 count, bool writable) {
        HC_ASSERT(count, "Must have something to allocate");
        return Result<HCBuffer, DeviceResult>();
    }

    void Device::destroy_buffer(const HCBuffer &buffer, u8 frame_mod) {

    }

    Result<HCDynamicBuffer, DeviceResult> Device::new_dynamic_buffer(HCBufferKind kind,
                                                                     resource::Descriptor &&descriptor, u64 count,
                                                                     bool writable) {
        HC_ASSERT(kind != HCBufferKind::Index,
                  "`new_dynamic_index_buffer` should be used to create dynamic index buffers");
        HC_ASSERT(count, "Must have something to allocate");
        return Result<HCDynamicBuffer, DeviceResult>();
    }

    Result<HCDynamicBuffer, DeviceResult> Device::new_dynamic_index_buffer(HCPrimitive index_type, u64 count,
                                                                           bool writable) {
        HC_ASSERT(count, "Must have something to allocate");
        return Result<HCDynamicBuffer, DeviceResult>();
    }

    void Device::destroy_dynamic_buffer(const HCDynamicBuffer &buffer, u8 frame_mod) {

    }
}
