#include <pch.hpp>

#include "device.hpp"

#include "../util.hpp"

#include <core/log.hpp>
#include <core/window.hpp>
#include <render/renderer.hpp>
#include <render/vars.hpp>
#include <util/flow.hpp>

namespace hc::render::device {
    std::optional<Device> Device::create(VkPhysicalDevice physical_handle, const std::vector<const char*>& layers) {
        Device device;
        vkGetPhysicalDeviceProperties(physical_handle, &device.properties);
        HC_INFO("Physical device found: " << device.properties.deviceName);
        vkGetPhysicalDeviceFeatures(physical_handle, &device.features);

        auto selection_res = Scheduler::select_queues(physical_handle);
        if (!selection_res) {
            return std::nullopt;
        }
        QueueSelection queue_selection = *std::move(selection_res);
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

        auto scheduler_res = Scheduler::create(device.fn_table, handle, queue_selection, max_frames_in_flight());
        if (!scheduler_res) {
            return std::nullopt;
        }
        device.scheduler = *std::move(scheduler_res);

        auto memory_res = memory::Memory::create(physical_handle, device.fn_table, handle, device.properties.limits);
        if (!memory_res) {
            HC_ERROR("Failed to create device memory");
            device.fn_table.vkDestroyDevice(handle, nullptr);
            return std::nullopt;
        }
        device.memory = std::move(memory_res).ok();

        device.graph = Graph::create(
            device.scheduler.graphics_queues()[0].get().family,
            device.scheduler.compute_queue().family,
            device.scheduler.transfer_queue().family
        );

        device.physical_handle = physical_handle;
        device.handle = handle;
        device.cleanup_queues = std::vector<std::vector<DestructionMark>>(max_frames_in_flight());

        return device;
    }

    Device::~Device() {
        if (this->handle != VK_NULL_HANDLE) {
            this->memory.destroy(this->fn_table, this->handle);
            this->scheduler.destroy(this->fn_table, this->handle);
            this->fn_table.vkDestroyDevice(this->handle, nullptr);
            this->physical_handle.destroy();
            this->handle.destroy();
        }
    }

    // TODO this should return a result
    void Device::tick(u8 frame_mod, u8 next_frame_mod) {
        this->cleanup(frame_mod);

        this->memory.unmap_ranges(this->fn_table, this->handle);
        memory::MemoryResult mem_res = this->memory.flush_ranges(this->fn_table, this->handle, frame_mod);
        if (mem_res != memory::MemoryResult::Success) {
            HC_ERROR("Failed to flush memory ranges");
            return;
        }

        GraphResult graph_res = this->graph.compile();
        HC_ASSERT(graph_res == GraphResult::Success, "Graph compilation should always succeed");

        // this->graph.record();

        this->present(frame_mod);

        mem_res = this->memory.map_ranges(this->fn_table, this->handle, next_frame_mod);
        if (mem_res != memory::MemoryResult::Success) {
            HC_ERROR("Failed to map memory ranges");
        }
    }

    void Device::finish(std::vector<u8> const& frame_mods) {
        this->memory.unmap_ranges(this->fn_table, this->handle);

        this->fn_table.vkDeviceWaitIdle(this->handle);

        for (u8 frame_mod : frame_mods) {
            this->cleanup(frame_mod);
        }
    }

    const char* Device::name() const noexcept {
        return this->properties.deviceName;
    }

    std::optional<VkSurfaceCapabilities2KHR> surface_capabilities(
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
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_handle, &surface_info, &capabilities);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface capabilities: " << to_str(res));
            return std::nullopt;
        }

        return capabilities;
    }

    std::vector<VkSurfaceFormat2KHR> surface_formats(VkPhysicalDevice physical_handle, const VkSurfaceKHR& surface) {
        u32 format_count = 0;

        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext = nullptr,
            .surface = surface,
        };

        VkResult res = vkGetPhysicalDeviceSurfaceFormats2KHR(
            physical_handle,
            &surface_info,
            &format_count,
            nullptr
        );
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface formats: " << to_str(res));
            return {};
        }
        std::vector<VkSurfaceFormat2KHR> formats(format_count);
        for (auto& format : formats) {
            format.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }
        res = vkGetPhysicalDeviceSurfaceFormats2KHR(
            physical_handle,
            &surface_info,
            &format_count,
            formats.data()
        );
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface formats: " << to_str(res));
            return {};
        }

        return formats;
    }

    std::vector<VkPresentModeKHR> surface_present_modes(VkPhysicalDevice physical_handle, const VkSurfaceKHR& surface) {
        u32 mode_count = 0;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_handle, surface, &mode_count, nullptr);
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display modes: " << to_str(res));
            return {};
        }
        std::vector<VkPresentModeKHR> present_modes(mode_count);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_handle,
            surface,
            &mode_count,
            present_modes.data()
        );
        if (res != VK_SUCCESS) {
            HC_ERROR("Failed to query surface display modes: " << to_str(res));
            return {};
        }

        // The specification requires that this present mode is supported, if the surface is supported
        if (present_modes.empty())
            present_modes.push_back(VK_PRESENT_MODE_FIFO_KHR);

        return present_modes;
    }

    std::expected<void, DeviceResult> Device::create_swapchain(
        GLFWwindow* window,
        ExternalHandle<VkSurfaceKHR, VK_NULL_HANDLE>&& surface,
        VkExtent2D extent
    ) {
        auto present_support_res = this->scheduler.present_support(this->physical_handle, surface);
        if (!present_support_res) {
            HC_ERROR("Presentation not supported for swapchain surface");
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return std::unexpected(DeviceResult::SwapchainFailure);
        }
        u32 queue_index = *present_support_res;

        auto capabilities = surface_capabilities(this->physical_handle, surface);
        if (!capabilities) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return std::unexpected(DeviceResult::VkFailure);
        }

        std::vector<VkSurfaceFormat2KHR> formats = surface_formats(this->physical_handle, surface);
        if (formats.empty()) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();
            return std::unexpected(DeviceResult::SurfaceFailure);
        }

        std::vector<VkPresentModeKHR> present_modes = surface_present_modes(this->physical_handle, surface);
        if (present_modes.empty()) {
            vkDestroySurfaceKHR(vk_instance(), surface, nullptr);
            surface.destroy();

            return std::unexpected(DeviceResult::SurfaceFailure);
        }

        SurfaceInfo surface_info = {
            .capabilities = *capabilities,
            .available_formats = std::move(formats),
            .available_present_modes = std::move(present_modes),
        };

        SwapchainParams params = {
            .extent = extent,
            .preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
            .preferred_format = {
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            },
        };

        auto swapchain = Swapchain::create(
            this->fn_table,
            this->handle,
            std::move(surface),
            std::move(surface_info),
            std::move(params)
        );
        if (swapchain) {
            this->queue_windows[queue_index].push_back(window);
            this->swapchains.emplace(window, *std::move(swapchain));
            return {};
        } else {
            return std::unexpected(DeviceResult::SwapchainFailure);
        }
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
        auto node = this->swapchains.extract(window);

        WindowDestructionMark mark = {
            .window = window,
            .swapchain = std::move(node.mapped()),
        };
        this->cleanup_submissions.emplace_back(std::move(mark));
    }

    Result<buffer::Params, DeviceResult> Device::new_buffer(
        HCBufferKind kind,
        resource::Descriptor&& descriptor,
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

        auto alloc_res = this->memory.alloc(this->fn_table, this->handle, flags, descriptor.size() * count);
        if (!alloc_res) {
            return Err(DeviceResult::AllocFailure);
        }
        buffer::Params params = {};
        params.size = alloc_res.ok().size;

        params.id = this->graph.add_resource(alloc_res.ok());

        return Ok(params);
    }

    Result<buffer::Params, DeviceResult> Device::new_index_buffer(HCPrimitive index_type, u64 count, bool writable) {
        HC_ASSERT(count, "Must have something to allocate");

        VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (writable)
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        auto alloc_res = this->memory.alloc(this->fn_table, this->handle, flags, resource::size_of(index_type));
        if (!alloc_res) {
            return Err(DeviceResult::AllocFailure);
        }
        buffer::Params params = {};
        params.size = alloc_res.ok().size;

        params.id = this->graph.add_resource(alloc_res.ok());

        return Ok(params);
    }

    Result<buffer::DynamicParams, DeviceResult> Device::new_dynamic_buffer(
        HCBufferKind kind,
        resource::Descriptor&& descriptor,
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

        auto alloc_res = this->memory.alloc_dyn(
            this->fn_table,
            this->handle,
            flags,
            descriptor.size() * count,
            frame_mod
        );
        if (!alloc_res) {
            return Err(DeviceResult::AllocFailure);
        }
        auto ref = std::move(alloc_res).ok();
        buffer::DynamicParams params = {};
        params.size = ref.size;
        params.data = ref.host_ptr;
        params.data_offset = ref.offset + ref.padding;

        params.id = this->graph.add_resource(ref);

        return Ok(params);
    }

    Result<buffer::DynamicParams, DeviceResult> Device::new_dynamic_index_buffer(
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

        auto alloc_res = this->memory.alloc_dyn(
            this->fn_table,
            this->handle,
            flags,
            resource::size_of(index_type),
            frame_mod
        );
        if (!alloc_res) {
            return Err(DeviceResult::AllocFailure);
        }
        auto ref = std::move(alloc_res).ok();
        buffer::DynamicParams params = {};
        params.size = ref.size;
        params.data = ref.host_ptr;
        params.data_offset = ref.offset + ref.padding;

        params.id = this->graph.add_resource(ref);

        return Ok(params);
    }

    void Device::destroy_buffer(u64 id) {
        this->cleanup_submissions.emplace_back(this->graph.remove_resource(id));
    }

    std::expected<texture::Params, DeviceResult> Device::create_texture(VkImageCreateInfo const& image_info) {
        auto texture_res = texture::create_image(this->physical_handle, this->fn_table, this->handle, image_info);
        if (!texture_res) {
            return std::unexpected(DeviceResult::TextureFailure);
        }
        VkImage image = *texture_res;

        auto ref_res = this->memory.alloc_texture(this->fn_table, this->handle, image);
        if (!ref_res) {
            return std::unexpected(DeviceResult::AllocFailure);
        }
        memory::Ref ref = *ref_res;

        texture::Params params = {};
        params.size = ref.size;
        params.id = this->graph.add_texture(ref, image, image_info);

        return params;
    }

    void Device::destroy_texture(u64 id) {
        this->cleanup_submissions.emplace_back(this->graph.remove_texture(id));
    }

    void Device::cleanup(u8 frame_mod) {
        auto& cleanup_queue = this->cleanup_queues[frame_mod];
        for (auto& mark : cleanup_queue) {
            std::visit(
                DestructionMarkHandler{
                    [this](WindowDestructionMark& window_mark) {
                        window_mark.swapchain.destroy(this->fn_table, this->handle);
                        window::destroy(window_mark.window);
                    },
                    [this](OldSwapchainDestructionMark const& swapchain_mark) {
                        HC_ASSERT(
                            this->swapchains.contains(swapchain_mark.window),
                            "A swapchain matching the mark's window should exist"
                        );
                        this->swapchains.at(swapchain_mark.window).destroy_old(this->fn_table, this->handle);
                    },
                    [this](ResourceDestructionMark const& resource_mark) {
                        this->memory.free(resource_mark);
                    },
                    [this](TextureDestructionMark const& texture_mark) {
                        this->fn_table.vkDestroyImage(this->handle, texture_mark.image, nullptr);
                        this->memory.free(texture_mark);
                    },
                },
                mark
            );
        }
        cleanup_queue.clear();
        std::swap(cleanup_queue, this->cleanup_submissions);
    }

    void Device::present(u8 frame_mod) {
        for (auto const& [graphics_queue_index, windows] : this->queue_windows) {
            Queue const& queue = this->scheduler.graphics_queues()[graphics_queue_index].get();
            VkCommandBuffer cmd_buffer = queue.pools[frame_mod].buffer;
            VkFence render_finished_fence = queue.pools[frame_mod].fence;
            VkSemaphore render_finished_semaphore = queue.pools[frame_mod].semaphore;

            VkResult res = fn_table.vkWaitForFences(
                this->handle,
                1,
                &render_finished_fence,
                VK_TRUE,
                UINT64_MAX
            );
            if (res != VK_SUCCESS) {
                HC_ERROR("Failed to wait for command buffer fence");
                // TODO proper error stuff
                return;;
            }

            std::vector<VkSwapchainKHR> swapchain_handles;
            std::vector<u32> image_indices;
            std::vector<VkSemaphore> image_semaphores;
            std::vector<VkPipelineStageFlags> semaphore_stage_flags;
            std::vector<VkRenderPassBeginInfo> render_pass_infos;
            std::vector<VkResult> presentation_results;
            std::vector<GLFWwindow*> unskipped_windows;

            swapchain_handles.reserve(this->swapchains.size());
            image_indices.reserve(this->swapchains.size());
            image_semaphores.reserve(this->swapchains.size());
            semaphore_stage_flags.reserve(this->swapchains.size());
            render_pass_infos.reserve(this->swapchains.size());
            presentation_results.reserve(this->swapchains.size());
            unskipped_windows.reserve(this->swapchains.size());

            for (auto const& window : windows) {
                auto& swapchain = this->swapchains.at(window);

                if (swapchain.is_out_of_date()) {
                    auto recreation_res = swapchain.recreate(
                        this->physical_handle,
                        this->fn_table,
                        this->handle,
                        window,
                        true
                    );

                    if (recreation_res && *recreation_res) {
                        this->cleanup_submissions.emplace_back(OldSwapchainDestructionMark{window});
                    } else {
                        continue;
                    }
                }

                auto image_details = swapchain.acquire_image(this->fn_table, this->handle, frame_mod);
                if (!image_details) {
                    continue;
                }
                auto [index, image_ready_semaphore] = *image_details;

                swapchain_handles.push_back(swapchain.handle());
                image_indices.push_back(index);
                image_semaphores.push_back(image_ready_semaphore);
                semaphore_stage_flags.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                render_pass_infos.push_back(swapchain.render_pass_info(index));
                presentation_results.push_back(VK_SUCCESS);
                unskipped_windows.push_back(window);
            }

            this->fn_table.vkResetCommandPool(this->handle, queue.pools[frame_mod].handle, 0);

            VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr,
            };
            this->fn_table.vkBeginCommandBuffer(cmd_buffer, &begin_info);

            for (auto const& render_pass_info : render_pass_infos) {
                this->fn_table.vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
                this->fn_table.vkCmdEndRenderPass(cmd_buffer);
            }

            this->fn_table.vkEndCommandBuffer(cmd_buffer);

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

            this->fn_table.vkResetFences(this->handle, 1, &render_finished_fence);
            this->fn_table.vkQueueSubmit(queue.handle, 1, &submit_info, render_finished_fence);

            if (!swapchain_handles.empty()) {
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

                res = this->fn_table.vkQueuePresentKHR(queue.handle, &present_info);
                if (res != VK_SUCCESS) {
                    for (const auto& [window, result] : std::views::zip(unskipped_windows, presentation_results)) {
                        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                            auto recreation_res = this->swapchains.at(window).recreate(
                                this->physical_handle,
                                this->fn_table,
                                this->handle,
                                window,
                                result == VK_ERROR_OUT_OF_DATE_KHR
                            );

                            if (recreation_res && *recreation_res) {
                                this->cleanup_submissions.emplace_back(OldSwapchainDestructionMark{window});
                            }
                        }
                    }
                }
            }
        }
    }
}
