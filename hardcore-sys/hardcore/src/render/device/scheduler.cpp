#include <pch.hpp>

#include <core/log.hpp>
#include <render/util.hpp>

#include "scheduler.hpp"

#include "util/flow.hpp"

namespace hc::render::device {
    std::expected<CommandPool, SchedulerError> CommandPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        u32 queue_family
    ) {
        CommandPool pool;

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family,
        };

        VkResult res = fn_table.vkCreateCommandPool(device, &pool_info, nullptr, &pool.handle.get());
        if (res != VK_SUCCESS) {
            switch (res) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                HC_ERROR("Failed to allocate command pool, out of host memory");
                return std::unexpected(SchedulerError::OutOfHostMemory);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                HC_ERROR("Failed to allocate command pool, out of device memory");
                return std::unexpected(SchedulerError::OutOfDeviceMemory);
            default: HC_UNREACHABLE("vkCreateCommandPool shouldn't return any other result values");
            }
        }

        VkCommandBufferAllocateInfo command_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool.handle,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        res = fn_table.vkAllocateCommandBuffers(device, &command_buffer_info, &pool.buffer.get());
        if (res != VK_SUCCESS) {
            pool.destroy(fn_table, device);

            switch (res) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                HC_ERROR("Failed to allocate command buffer, out of host memory");
                return std::unexpected(SchedulerError::OutOfHostMemory);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                HC_ERROR("Failed to allocate command buffer, out of device memory");
                return std::unexpected(SchedulerError::OutOfDeviceMemory);
            default: HC_UNREACHABLE("vkAllocateCommandBuffers shouldn't return any other result values");
            }
        }

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        res = fn_table.vkCreateFence(device, &fence_info, nullptr, &pool.fence.get());
        if (res != VK_SUCCESS) {
            pool.destroy(fn_table, device);

            switch (res) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                HC_ERROR("Failed to allocate fence, out of host memory");
                return std::unexpected(SchedulerError::OutOfHostMemory);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                HC_ERROR("Failed to allocate fence, out of device memory");
                return std::unexpected(SchedulerError::OutOfDeviceMemory);
            default: HC_UNREACHABLE("vkCreateFence shouldn't return any other result values");
            }
        }

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };

        res = fn_table.vkCreateSemaphore(device, &semaphore_info, nullptr, &pool.semaphore.get());
        if (res != VK_SUCCESS) {
            pool.destroy(fn_table, device);

            switch (res) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                HC_ERROR("Failed to allocate fence, out of host memory");
                return std::unexpected(SchedulerError::OutOfHostMemory);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                HC_ERROR("Failed to allocate fence, out of device memory");
                return std::unexpected(SchedulerError::OutOfDeviceMemory);
            default: HC_UNREACHABLE("vkCreateFence shouldn't return any other result values");
            }
        }

        return pool;
    }

    void CommandPool::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
        if (this->semaphore.valid()) {
            fn_table.vkDestroySemaphore(device, this->semaphore, nullptr);
            this->semaphore.destroy();
        }

        if (this->fence.valid()) {
            fn_table.vkDestroyFence(device, this->fence, nullptr);
            this->fence.destroy();
        }

        if (this->buffer.valid()) {
            fn_table.vkFreeCommandBuffers(device, this->handle, 1, &this->buffer.get());
            this->buffer.destroy();
        }

        fn_table.vkDestroyCommandPool(device, this->handle, nullptr);
        this->handle.destroy();
    }

    std::expected<QueueSelection, SchedulerError> Scheduler::select_queues(VkPhysicalDevice physical_device) {
        QueueSelection selection;
        selection.compute_family = std::numeric_limits<u32>::max();
        selection.transfer_family = std::numeric_limits<u32>::max();

        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        selection.family_properties = std::vector(queue_family_count, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device,
            &queue_family_count,
            selection.family_properties.data()
        );

        u32 compute_score = 0;
        u32 transfer_score = 0;

        for (u32 family = 0; family < selection.family_properties.size(); family++) {
            // Does this even happen?
            if (!selection.family_properties[family].queueCount) {
                continue;
            }

            if (selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // Graphics queue is selected at runtime, depending on target surfaces
                selection.graphics_families.push_back(family);
            }

            if (compute_score < 1 && (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                selection.compute_family = family;
                compute_score = 1;
            }

            if (compute_score < 2 && (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // Async compute queue is optimal
                selection.compute_family = family;
                compute_score = 2;
            }

            if (transfer_score < 1 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                selection.transfer_family = family;
                transfer_score = 1;
            }

            if (transfer_score < 2 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                selection.transfer_family = family;
                transfer_score = 2;
            }

            if (transfer_score < 3 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(selection.family_properties[family].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
                // Async transfer is optimal for host<->device transfers
                selection.transfer_family = family;
                transfer_score = 3;
            }

            HC_TRACE(
                "Queue family " << family << " properties: Count: " << selection.family_properties[family].queueCount << "\tFlags: "
                << (selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "GRAPHICS | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT ? "COMPUTE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT ? "TRANSFER | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SPARSE_BINDING | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_PROTECTED_BIT ? "PROTECTED | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? "VIDEO_DECODE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ? "VIDEO_ENCODE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ? "OPTICAL_FLOW | " : "")
                << (selection.family_properties[family].queueFlags ? "\b\b  " : "NONE")
            );
        }

        if (selection.graphics_families.empty()) {
            HC_ERROR("No graphics queue families found");
            return std::unexpected(SchedulerError::NoGraphicsQueueFound);
        }

#ifdef HC_LOGGING
        std::stringstream stream;
        stream << "Selected graphics queues: ";
        for (u32 family : selection.graphics_families) {
            stream << family << ", ";
        }
        stream << "\b\b  ";
        HC_DEBUG(stream.str());
#endif // HC_LOGGING

        if (selection.compute_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected compute queue family index: " << selection.compute_family);
        } else {
            HC_WARN("No compute queue family found");
            return std::unexpected(SchedulerError::NoComputeQueueFound);
        }

        if (selection.transfer_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected transfer queue family index: " << selection.transfer_family);
        } else {
            HC_ERROR("No transfer queue family found");
            return std::unexpected(SchedulerError::NoTransferQueueFound);
        }

        return selection;
    }

    std::expected<Scheduler, SchedulerError> Scheduler::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        QueueSelection const& selection,
        u8 max_frames_in_flight
    ) {
        Scheduler scheduler;

        std::set<u32> unique_families;
        unique_families.insert(selection.graphics_families.begin(), selection.graphics_families.end());
        unique_families.insert(selection.compute_family);
        unique_families.insert(selection.transfer_family);

        for (u32 const queue_family : unique_families) {
            u32 const index = static_cast<u32>(scheduler.queues.size());

            auto found = std::ranges::find(selection.graphics_families, queue_family);
            if (found != selection.graphics_families.end()) {
                scheduler.graphics_queue_indexes.push_back(index);
            }

            if (selection.compute_family == queue_family) {
                scheduler.compute_queue_index = queue_family;
            }

            if (selection.transfer_family == queue_family) {
                scheduler.transfer_queue_index = queue_family;
            }

            VkQueue handle = VK_NULL_HANDLE;
            fn_table.vkGetDeviceQueue(device, queue_family, 0, &handle);

            std::vector<CommandPool> pools;
            pools.reserve(max_frames_in_flight);

            for (u8 i = 0; i < max_frames_in_flight; ++i) {
                auto pool_res = CommandPool::create(fn_table, device, queue_family);
                if (!pool_res) {
                    for (auto& pool : pools) {
                        pool.destroy(fn_table, device);
                    }
                    pools.clear();

                    scheduler.destroy(fn_table, device);

                    return std::unexpected(pool_res.error());
                }
                pools.push_back(*std::move(pool_res));
            }

            scheduler.queues.push_back(
                {
                    .handle = handle,
                    .family = queue_family,
                    .family_properties = selection.family_properties[queue_family],
                    .pools = std::move(pools),
                }
            );
        }

        for (u32 index : scheduler.graphics_queue_indexes) {
            scheduler.graphics_queue_refs.push_back(std::ref(scheduler.queues[index]));
        }

        return scheduler;
    }

    void Scheduler::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (Queue& queue : this->queues) {
            for (auto& pool : queue.pools) {
                pool.destroy(fn_table, device);
            }

            queue.pools.clear();

            // Queues aren't freed
            queue.handle.destroy();
        }

        this->queues.clear();
    }

    std::optional<u32> Scheduler::present_support(
        VkPhysicalDevice const& physical_handle,
        VkSurfaceKHR const& surface
    ) const {
        for (u32 i = 0; i < this->graphics_queue_indexes.size(); i++) {
            u32 const family = this->queues[this->graphics_queue_indexes[i]].family;
            VkBool32 supported = VK_FALSE;
            VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_handle,
                family,
                surface,
                &supported
            );

            if (res != VK_SUCCESS) {
                HC_ERROR("Failed to query queue family surface support for queue family " << family << ": " << to_str(res));
                continue;
            }

            if (supported == VK_TRUE) {
                return i;
            }
        }

        return std::nullopt;
    }

    std::expected<void, SchedulerError> Scheduler::reset_pools(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        u8 frame_mod
    ) {
        for (auto& queue : this->queues) {
            VkResult res = fn_table.vkResetCommandPool(device, queue.pools[frame_mod].handle, 0);

            if (res != VK_SUCCESS) {
                return std::unexpected(SchedulerError::OutOfDeviceMemory);
            }
        }

        return {};
    }
}
