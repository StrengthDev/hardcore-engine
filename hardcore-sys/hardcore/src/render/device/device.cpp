#include <pch.hpp>

#include "device.hpp"

#include "../util.hpp"

#include <core/log.hpp>
#include <render/renderer.hpp>
#include <render/vars.hpp>
#include <util/flow.hpp>

namespace hc::render::device {
    static std::expected<VkSurfaceCapabilities2KHR, Error> surface_capabilities(
        VkPhysicalDevice physical_handle,
        const VkSurfaceKHR& surface
    ) {
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
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_handle, &surface_info, &capabilities);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface capabilities: " << to_str(result));
            return Error(result);
        }

        return capabilities;
    }

    static std::expected<std::vector<VkSurfaceFormat2KHR>, Error> surface_formats(
        VkPhysicalDevice physical_handle,
        const VkSurfaceKHR& surface
    ) {
        u32 format_count = 0;

        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext = nullptr,
            .surface = surface,
        };

        VkResult result = vkGetPhysicalDeviceSurfaceFormats2KHR(
            physical_handle,
            &surface_info,
            &format_count,
            nullptr
        );
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface format count: " << to_str(result));
            return Error(result);
        }
        std::vector<VkSurfaceFormat2KHR> formats(format_count);
        for (auto& format : formats) {
            format.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }
        result = vkGetPhysicalDeviceSurfaceFormats2KHR(
            physical_handle,
            &surface_info,
            &format_count,
            formats.data()
        );
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface formats: " << to_str(result));
            return Error(result);
        }

        return formats;
    }

    static std::expected<std::vector<VkPresentModeKHR>, Error> surface_present_modes(
        VkPhysicalDevice physical_handle,
        const VkSurfaceKHR& surface
    ) {
        u32 mode_count = 0;
        VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_handle, surface, &mode_count, nullptr);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display mode count: " << to_str(result));
            return Error(result);
        }
        std::vector<VkPresentModeKHR> present_modes(mode_count);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_handle,
            surface,
            &mode_count,
            present_modes.data()
        );
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display modes: " << to_str(result));
            return Error(result);
        }

        // The specification requires that this present mode is supported, if the surface is supported
        if (present_modes.empty()) {
            present_modes.push_back(VK_PRESENT_MODE_FIFO_KHR);
        }

        return present_modes;
    }

    std::expected<Device, Error> Device::create(VkPhysicalDevice physical_handle, const std::vector<const char*>& layers) {
        Device device;
        vkGetPhysicalDeviceProperties(physical_handle, &device.properties);
        HC_INFO("Physical device found: " << device.properties.deviceName);
        vkGetPhysicalDeviceFeatures(physical_handle, &device.features);

        auto selection_result = Scheduler::select_queues(physical_handle);
        if (!selection_result) {
            return selection_result.error();
        }
        QueueSelection queue_selection = *std::move(selection_result);
        std::set<u32> unique_queue_families;
        unique_queue_families.insert(queue_selection.graphics_families.begin(), queue_selection.graphics_families.end());
        unique_queue_families.insert(queue_selection.compute_family);
        unique_queue_families.insert(queue_selection.transfer_family);

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        queue_infos.reserve(unique_queue_families.size());
        float queue_priority = 1.0f;
        for (u32 index : unique_queue_families) {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = index;
            info.queueCount = 1;
            info.pQueuePriorities = &queue_priority;
            queue_infos.push_back(info);
        }

        VkPhysicalDeviceFeatures features = {};

        std::vector<const char*> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VkDeviceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = static_cast<u32>(queue_infos.size()),
            .pQueueCreateInfos = queue_infos.data(),
            .enabledLayerCount = static_cast<u32>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<u32>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = &features,
        };

        VkDevice handle;
        VkResult vk_result = vkCreateDevice(physical_handle, &create_info, nullptr, &handle);
        if (vk_result != VK_SUCCESS) {
            HC_ERROR("Failed to create Vulkan logical device: " << to_str(vk_result));
            return Error(vk_result);
        }

        volkLoadDeviceTable(&device.fn_table, handle);

        auto scheduler_result = Scheduler::create(device.fn_table, handle, queue_selection, max_frames_in_flight());
        if (!scheduler_result) {
            return scheduler_result.error();
        }
        device.scheduler = *std::move(scheduler_result);

        auto memory_result = memory::Memory::create(physical_handle, device.fn_table, handle, device.properties.limits);
        if (!memory_result) {
            device.fn_table.vkDestroyDevice(handle, nullptr);
            return memory_result.error();
        }
        device.memory = *std::move(memory_result);

        VkPipelineCacheCreateInfo cache_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .initialDataSize = 0,
            .pInitialData = nullptr
        };

        vk_result = device.fn_table.vkCreatePipelineCache(handle, &cache_info, nullptr, &device.pipeline_cache.get());
        if (vk_result != VK_SUCCESS) {
            HC_ERROR("Failed to create pipeline cache: " << to_str(vk_result));
            device.memory.destroy(device.fn_table, handle);
            device.fn_table.vkDestroyDevice(handle, nullptr);
            return Error(vk_result);
        }

        device.graph = Graph::create(
            device.scheduler.graphics_queues()[0].get().family,
            device.scheduler.compute_queue().family,
            device.scheduler.transfer_queue().family
        );

        device.physical_handle = physical_handle;
        device.handle = handle;

        return device;
    }

    Device::~Device() {
        if (this->handle.valid()) {
            this->graph.destroy(this->fn_table, this->handle);

            this->fn_table.vkDestroyPipelineCache(this->handle, this->pipeline_cache, nullptr);
            this->pipeline_cache.destroy();

            this->memory.destroy(this->fn_table, this->handle);
            this->scheduler.destroy(this->fn_table, this->handle);
            this->fn_table.vkDestroyDevice(this->handle, nullptr);
            this->physical_handle.destroy();
            this->handle.destroy();
        }
    }

    std::expected<void, Error> Device::tick(u8 frame_mod, u8 next_frame_mod) {
        this->cleaner.tick(this->fn_table, this->handle, this->memory);

        this->memory.unmap_ranges(this->fn_table, this->handle);
        auto memory_result = this->memory.flush_ranges(this->fn_table, this->handle, frame_mod);
        if (!memory_result) {
            return memory_result.error();
        }

        auto graph_result = this->graph.compile();
        if (!graph_result) {
            return graph_result.error();
        }

        // this->graph.record();

        auto present_result = this->present(frame_mod);
        if (!present_result) {
            return present_result.error();
        }

        memory_result = this->memory.map_ranges(this->fn_table, this->handle, next_frame_mod);
        if (!memory_result) {
            return memory_result.error();
        }

        return {};
    }

    void Device::finish() {
        this->memory.unmap_ranges(this->fn_table, this->handle);

        this->fn_table.vkDeviceWaitIdle(this->handle);

        this->cleaner.clear(this->fn_table, this->handle, this->memory);
    }

    const char* Device::name() const noexcept {
        return this->properties.deviceName;
    }

    std::expected<void, Error> Device::create_swapchain(
        GLFWwindow* window,
        ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE>&& surface,
        VkExtent2D extent
    ) {
        auto queue_index_opt = this->scheduler.present_support(this->physical_handle, surface);
        if (!queue_index_opt) {
            HC_ERROR("Presentation not supported for swapchain surface");
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return Error(HCError_NoPresentSupport);
        }

        auto capabilities_result = surface_capabilities(this->physical_handle, surface);
        if (!capabilities_result) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return capabilities_result.error();
        }

        auto formats_result = surface_formats(this->physical_handle, surface);
        if (!formats_result) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return formats_result.error();
        }

        auto present_modes_result = surface_present_modes(this->physical_handle, surface);
        if (!present_modes_result) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return present_modes_result.error();
        }

        swapchain::SurfaceInfo surface_info = {
            .capabilities = *capabilities_result,
            .available_formats = *std::move(formats_result),
            .available_present_modes = *std::move(present_modes_result),
        };

        swapchain::SwapchainParams params = {
            .extent = extent,
            .preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
            .preferred_format = {
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            },
        };

        auto swapchain_result = swapchain::Swapchain::create(
            this->fn_table,
            this->handle,
            std::move(surface),
            std::move(surface_info),
            std::move(params)
        );
        if (!swapchain_result) {
            return swapchain_result.error();
        }

        this->queue_windows[*queue_index_opt].push_back(window);
        this->swapchains.emplace(window, *std::move(swapchain_result));
        return {};
    }

    void Device::destroy_swapchain(GLFWwindow* window) {
        const auto it = std::ranges::find_if(
            this->queue_windows,
            [window](auto const& value) {
                return std::ranges::find(value.second, window) != value.second.end();
            }
        );

        HC_ASSERT(it != this->queue_windows.end(), "A swapchain matching the window must exist");
        u32 queue_index = it->first;
        std::erase(this->queue_windows[queue_index], window);
        if (this->queue_windows[queue_index].empty()) {
            this->queue_windows.erase(queue_index);
        }

        auto node = this->swapchains.extract(window);

        this->cleaner.yield_window(window, std::move(node.mapped()));
    }

    std::expected<buffer::BufferData, Error> Device::new_buffer(
        HCBufferKind kind,
        Descriptor&& descriptor,
        u64 count,
        bool writable
    ) {
        HC_ASSERT(
            kind != HCBufferKind::HCBufferKind_Index,
            "`new_index_buffer` should be used to create index buffers"
        );
        HC_ASSERT(count, "Must have something to allocate");

        VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        switch (kind) {
        case HCBufferKind_Vertex:
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case HCBufferKind_Uniform:
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case HCBufferKind_Storage:
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
        default: HC_UNREACHABLE("No other resource kind should appear here");
        }
        if (writable)
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        auto alloc_result = this->memory.alloc_buffer(this->fn_table, this->handle, flags, descriptor.size() * count);
        if (!alloc_result) {
            return alloc_result.error();
        }
        auto ref = *std::move(alloc_result);

        buffer::BufferData params = {
            .id = this->graph.add_resource(buffer::Buffer(ref)),
            .size = ref.size
        };

        return params;
    }

    std::expected<buffer::BufferData, Error> Device::new_index_buffer(HCPrimitive index_type, u64 count, bool writable) {
        HC_ASSERT(count, "Must have something to allocate");

        VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (writable)
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        auto alloc_result = this->memory.alloc_buffer(this->fn_table, this->handle, flags, size_of(index_type));
        if (!alloc_result) {
            return alloc_result.error();
        }
        auto ref = *std::move(alloc_result);

        buffer::BufferData params = {
            .id = this->graph.add_resource(buffer::Buffer(ref)),
            .size = ref.size
        };

        return params;
    }

    std::expected<buffer::DynamicBufferData, Error> Device::new_dynamic_buffer(
        HCBufferKind kind,
        Descriptor&& descriptor,
        u64 count,
        bool writable,
        u8 frame_mod
    ) {
        HC_ASSERT(
            kind != HCBufferKind::HCBufferKind_Index,
            "`new_dynamic_index_buffer` should be used to create dynamic index buffers"
        );
        HC_ASSERT(count, "Must have something to allocate");

        VkBufferUsageFlags flags = 0;
        switch (kind) {
        case HCBufferKind_Vertex:
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case HCBufferKind_Uniform:
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case HCBufferKind_Storage:
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
        default: HC_UNREACHABLE("No other resource kind should appear here");
        }
        if (writable) {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        auto alloc_result = this->memory.alloc_buffer_dyn(
            this->fn_table,
            this->handle,
            flags,
            descriptor.size() * count,
            frame_mod
        );
        if (!alloc_result) {
            return alloc_result.error();
        }
        auto ref = *std::move(alloc_result);
        buffer::DynamicBufferData data = {
            .id = this->graph.add_resource(buffer::DynamicBuffer(ref)),
            .size = ref.size,
            .map_ptr = ref.host_ptr,
            .map_offset = ref.offset + ref.padding,
        };

        return data;
    }

    std::expected<buffer::DynamicBufferData, Error> Device::new_dynamic_index_buffer(
        HCPrimitive index_type,
        u64 count,
        bool writable,
        u8 frame_mod
    ) {
        HC_ASSERT(count, "Must have something to allocate");

        VkBufferUsageFlags flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (writable) {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        auto alloc_result = this->memory.alloc_buffer_dyn(
            this->fn_table,
            this->handle,
            flags,
            size_of(index_type),
            frame_mod
        );
        if (!alloc_result) {
            return alloc_result.error();
        }
        auto ref = *std::move(alloc_result);
        buffer::DynamicBufferData data = {
            .id = this->graph.add_resource(buffer::DynamicBuffer(ref)),
            .size = ref.size,
            .map_ptr = ref.host_ptr,
            .map_offset = ref.offset + ref.padding,
        };

        return data;
    }

    void Device::destroy_buffer(u64 id) {
        // todo check if buffer exists
        this->cleaner.yield_buffer(this->graph.remove_resource_b(id));
    }

    void Device::destroy_dynamic_buffer(u64 id) {
        // todo check if buffer exists
        this->cleaner.yield_dynamic_buffer(this->graph.remove_resource_bd(id));
    }

    std::expected<texture::TextureData, Error> Device::create_texture(VkImageCreateInfo const& image_info) {
        auto texture_result = texture::create_image(this->physical_handle, this->fn_table, this->handle, image_info);
        if (!texture_result) {
            return texture_result.error();
        }
        VkImage image = *texture_result;

        auto ref_result = this->memory.alloc_texture(this->fn_table, this->handle, image);
        if (!ref_result) {
            return ref_result.error();
        }
        memory::Ref ref = *ref_result;

        texture::TextureData data = {
            .id = this->graph.add_texture(texture::Texture(ref, image, image_info)),
            .size = ref.size,
        };

        return data;
    }

    void Device::destroy_texture(u64 id) {
        this->cleaner.yield_texture(this->graph.remove_texture(id));
    }

    std::expected<u64, Error> Device::create_render_pass(
        std::span<HCSubpass const> const& subpasses,
        UserPredicate<Sz>&& predicate
    ) {
        return this->graph.create_render_pass(this->fn_table, this->handle, subpasses, std::move(predicate));
    }

    void Device::destroy_render_pass(u64 id) {
        this->graph.destroy_render_pass(this->fn_table, this->handle, id);
    }

    std::expected<void, Error> Device::present(u8 frame_mod) {
        std::expected<void, Error> return_result = {};

        for (auto const& [graphics_queue_index, windows] : this->queue_windows) {
            Queue const& queue = this->scheduler.graphics_queues()[graphics_queue_index].get();
            auto result = this->present_queue_windows(frame_mod, queue, windows);
            if (!result && !return_result) {
                return_result = result;
            }
        }

        return return_result;
    }

    std::expected<void, Error> Device::present_queue_windows(u8 frame_mod, Queue const& queue, const std::vector<GLFWwindow*>& windows) {
        VkCommandBuffer cmd_buffer = queue.pools[frame_mod].buffer;
        VkFence render_finished_fence = queue.pools[frame_mod].fence;
        VkSemaphore render_finished_semaphore = queue.pools[frame_mod].semaphore;

        VkResult result = fn_table.vkWaitForFences(
            this->handle,
            1,
            &render_finished_fence,
            VK_TRUE,
            UINT64_MAX
        );
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to wait for command buffer fence: " << to_str(result));
            return Error(result);
        }

        std::vector<VkSwapchainKHR> swapchain_handles;
        std::vector<u32> image_indices;
        std::vector<VkSemaphore> image_semaphores;
        std::vector<VkRenderPassBeginInfo> render_pass_infos;
        std::vector<GLFWwindow*> unskipped_windows;

        swapchain_handles.reserve(windows.size());
        image_indices.reserve(windows.size());
        image_semaphores.reserve(windows.size());
        render_pass_infos.reserve(windows.size());
        unskipped_windows.reserve(windows.size());

        for (auto const& window : windows) {
            auto& swapchain = this->swapchains.at(window);

            if (swapchain.is_out_of_date()) {
                auto recreation_result = swapchain.recreate(
                    this->physical_handle,
                    this->fn_table,
                    this->handle,
                    window,
                    true
                );

                if (recreation_result) {
                    if (*recreation_result) {
                        this->cleaner.yield_swapchain(**std::move(recreation_result));
                    } else {
                        continue;
                    }
                } else {
                    return Error(result);
                }
            }

            auto image_details = swapchain.acquire_image(this->fn_table, this->handle, frame_mod);
            if (!image_details) {
                return image_details.error();
            }
            auto [image_ready_semaphore, index, acquisition] = *image_details;

            if (acquisition != swapchain::AcquisitionKind::Normal) {
                continue;
            }

            swapchain_handles.push_back(swapchain.handle());
            image_indices.push_back(index);
            image_semaphores.push_back(image_ready_semaphore);
            render_pass_infos.push_back(swapchain.render_pass_info(index));
            unskipped_windows.push_back(window);
        }

        if (swapchain_handles.empty()) {
            return {};
        }

        // TODO reset, begin and end should probably not be here

        result = this->fn_table.vkResetCommandPool(this->handle, queue.pools[frame_mod].handle, 0);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to reset command pool: " << to_str(result));
            return Error(result);
        }

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };

        result = this->fn_table.vkBeginCommandBuffer(cmd_buffer, &begin_info);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to begin swapchain target drawing command buffer: " << to_str(result));
            return Error(result);
        }

        for (auto const& render_pass_info : render_pass_infos) {
            this->fn_table.vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            this->fn_table.vkCmdEndRenderPass(cmd_buffer);
        }

        result = this->fn_table.vkEndCommandBuffer(cmd_buffer);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to end swapchain target drawing command buffer: " << to_str(result));
            // This may return VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR, but the respective extension is not used
            return Error(result);
        }

        std::vector<VkPipelineStageFlags> semaphore_stage_flags(
            image_semaphores.size(),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<u32>(image_semaphores.size()),
            .pWaitSemaphores = image_semaphores.data(),
            .pWaitDstStageMask = semaphore_stage_flags.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            .signalSemaphoreCount = swapchain_handles.empty() ? 0U : 1U,
            .pSignalSemaphores = swapchain_handles.empty() ? nullptr : &render_finished_semaphore,
        };

        result = this->fn_table.vkResetFences(this->handle, 1, &render_finished_fence);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to reset pool fence: " << to_str(result));
            return Error(result);
        }

        result = this->fn_table.vkQueueSubmit(queue.handle, 1, &submit_info, render_finished_fence);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to submit swapchain target drawing commands: " << to_str(result));
            return Error(result);
        }

        std::vector<VkResult> presentation_results(swapchain_handles.size(), VK_SUCCESS);

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = static_cast<u32>(swapchain_handles.size()),
            .pSwapchains = swapchain_handles.data(),
            .pImageIndices = image_indices.data(),
            .pResults = presentation_results.data(),
        };

        result = this->fn_table.vkQueuePresentKHR(queue.handle, &present_info);
        if (result != VK_SUCCESS) {
            std::expected<void, Error> error = {};

            for (const auto& [window, presentation_result] : std::views::zip(unskipped_windows, presentation_results)) {
                if (presentation_result == VK_ERROR_OUT_OF_DATE_KHR || presentation_result == VK_SUBOPTIMAL_KHR) {
                    auto recreation_result = this->swapchains.at(window).recreate(
                        this->physical_handle,
                        this->fn_table,
                        this->handle,
                        window,
                        presentation_result == VK_ERROR_OUT_OF_DATE_KHR
                    );

                    if (recreation_result && *recreation_result) {
                        this->cleaner.yield_swapchain(**std::move(recreation_result));
                    }
                } else {
                    HC_ERROR("Failed to present swapchain image: " << to_str(presentation_result));
                    if (!error) {
                        error = Error(presentation_result);
                    }
                }
            }

            return error;
        }

        return {};
    }
}
