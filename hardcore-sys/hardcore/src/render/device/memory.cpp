#include <pch.hpp>

#include "memory.hpp"

#include <render/device.hpp>

#include <core/log.hpp>

/*
namespace hc::device {
    const VkDeviceSize staging_buffer_size = MEGABYTES(8);

    template<device_memory::buffer_t BType>
    struct buffer {
        static_assert(BType >= device_memory::buffer_t::ENUM_COUNT, "Unimplemented buffer type");
    };
    template<>
    struct buffer<device_memory::buffer_t::VERTEX> {
        static constexpr const char *debug_name = "VERTEX";
        static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        static const VkDeviceSize size = MEGABYTES(8);
        static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        static const VkDeviceSize dynamic_size = MEGABYTES(8);
    };
    template<>
    struct buffer<device_memory::buffer_t::INDEX> {
        static constexpr const char *debug_name = "INDEX";
        static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        static const VkDeviceSize size = MEGABYTES(8);
        static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        static const VkDeviceSize dynamic_size = MEGABYTES(8);
    };
    template<>
    struct buffer<device_memory::buffer_t::UNIFORM> {
        static constexpr const char *debug_name = "UNIFORM";
        static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        static const VkDeviceSize size = MEGABYTES(8);
        static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        static const VkDeviceSize dynamic_size = MEGABYTES(8);
    };
    template<>
    struct buffer<device_memory::buffer_t::STORAGE> {
        static constexpr const char *debug_name = "STORAGE";
        static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        static const VkDeviceSize size = MEGABYTES(8);
        static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        static const VkDeviceSize dynamic_size = MEGABYTES(8);
    };
    template<>
    struct buffer<device_memory::buffer_t::UNIVERSAL> {
        static constexpr const char *debug_name = "UNIVERSAL";
        static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        static const VkDeviceSize size = MEGABYTES(8);
        static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        static const VkDeviceSize dynamic_size = MEGABYTES(8);
    };
    template<>
    struct buffer<device_memory::buffer_t::TEXTURE> {
        static constexpr const char *debug_name = "TEXTURE";
        static const VkDeviceSize size = MEGABYTES(128);
        static const VkDeviceSize dynamic_size = MEGABYTES(128);
    };

    template<bool Dynamic>
    struct get_buffer_pool {
    };
    template<>
    struct get_buffer_pool<false> {
        using type = buffer_pool;
    };
    template<>
    struct get_buffer_pool<true> {
        using type = dynamic_buffer_pool;
    };

    template<bool Dynamic>
    struct get_texture_pool {
    };
    template<>
    struct get_texture_pool<false> {
        using type = texture_pool;
    };
    //template<> struct get_texture_pool<true> { using type = dynamic_texture_pool; };

    struct internal_ref {
        u8 pool_type = 0;
        u32 pool = std::numeric_limits<u32>::max();
        std::size_t offset = std::numeric_limits<std::size_t>::max();
        std::size_t size = 0;
    };

    class mem_ref_internal : protected memory_ref {
    public:
        static inline memory_ref create(u8 pool_type, u32 pool, std::size_t offset, std::size_t size) noexcept {
            return mem_ref_internal(pool_type, pool, offset, size);
        }

        static inline internal_ref extract(const memory_ref &ref) noexcept {
            const mem_ref_internal &iref = static_cast<const mem_ref_internal &>(ref);
            return {iref.m_pool_type, iref.m_pool, iref.m_offset, iref.m_size};
        }

    private:
        mem_ref_internal(u8 pool_type, u32 pool, std::size_t offset, std::size_t size) noexcept:
                memory_ref(pool_type, pool, offset, size) {}
    };

    class mesh_internal : protected mesh {
    public:
        static inline internal_ref extract_index_ref(const mesh &mesh) noexcept {
            const mesh_internal &imesh = static_cast<const mesh_internal &>(mesh);
            return mem_ref_internal::extract(imesh.index_ref);
        }
    };

    class resource_internal : protected resource {
    public:
        static inline internal_ref extract_ref(const resource &res) noexcept {
            const resource_internal &ires = static_cast<const resource_internal &>(res);
            return mem_ref_internal::extract(ires.ref);
        }
    };

    inline VkDeviceSize increase_to_fit(VkDeviceSize base, VkDeviceSize target) {
        // Mathematical equivalent to a loop doubling base until it can fit target
        const double exp = std::ceil(std::log2(static_cast<double>(target) / base));
        return base * std::exp2(exp);
    }

    template<typename Pool>
    inline bool search_buffer_pools(const std::vector<Pool> &pools, VkDeviceSize size, VkDeviceSize alignment,
                                    u32 &out_pool_idx, u32 &out_slot_idx, VkDeviceSize &out_size_needed,
                                    VkDeviceSize &out_offset) {
        static_assert(std::is_base_of<BufferPool, Pool>::value, "Invalid pool type");

        u32 selected_pool_idx = 0;
        u32 selected_slot_idx = std::numeric_limits<u32>::max();
        for (const buffer_pool &pool: pools) {
            if (pool.search(size, alignment, selected_slot_idx, out_size_needed, out_offset))
                break;

            selected_pool_idx++;
        }

        out_pool_idx = selected_pool_idx;

        // Did not find valid slot, need to allocate new pool, set out variables to the first slot of the new pool
        if (selected_slot_idx == std::numeric_limits<u32>::max()) {
            out_slot_idx = 0;
            out_size_needed = size;
            out_offset = 0;
            return false;
        }

            // Found a valid slot, only need to set selected slot
        else {
            out_slot_idx = selected_slot_idx;
            return true;
        }
    }

    template<typename Pool>
    inline bool search_texture_pools(const std::vector<Pool> &pools, const VkMemoryRequirements &requirements,
                                     u32 &out_pool_idx, u32 &out_slot_idx, VkDeviceSize &out_size_needed,
                                     VkDeviceSize &out_offset) {
        static_assert(std::is_base_of<texture_pool, Pool>::value, "Invalid pool type");

        u32 selected_pool_idx = 0;
        u32 selected_slot_idx = std::numeric_limits<u32>::max();
        for (const texture_pool &pool: pools) {
            if (pool.search(requirements.size, requirements.alignment, requirements.memoryTypeBits,
                            selected_slot_idx, out_size_needed, out_offset))
                break;

            selected_pool_idx++;
        }

        out_pool_idx = selected_pool_idx;

        // Did not find valid slot, need to allocate new pool, set out variables to the first slot of the new pool
        if (selected_slot_idx == std::numeric_limits<u32>::max()) {
            out_slot_idx = 0;
            out_size_needed = requirements.size;
            out_offset = 0;
            return false;
        }

            // Found a valid slot, only need to set selected slot
        else {
            out_slot_idx = selected_slot_idx;
            return true;
        }
    }

    void device_memory::init(VkPhysicalDevice physical_device, VkDevice device, u32 transfer_queue_idx,
                             const VkPhysicalDeviceLimits *limits, const u8 *current_frame) {
        update_refs(device, limits, current_frame);

        heap_manager = device_heap_manager(physical_device);

        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = transfer_queue_idx;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CRASH_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool), "Failed to create command pool");

        VkCommandBufferAllocateInfo cmd_buffer_info = {};
        cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buffer_info.commandPool = cmd_pool;
        cmd_buffer_info.commandBufferCount = max_frames_in_flight;

        VK_CRASH_CHECK(vkAllocateCommandBuffers(device, &cmd_buffer_info, cmd_buffers.data()),
                       "Failed to allocate command buffers");

        //texture_pools.resize(heap_manager.mem_properties().memoryTypeCount);

        m_upload_pools.emplace_back(device, heap_manager, staging_buffer_size);
        m_tex_upload_pools.emplace_back(device, heap_manager, staging_buffer_size);

        for (u32 i = 0; i < max_frames_in_flight; i++) {
            VkSemaphoreCreateInfo semaphore_info = {};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VK_CRASH_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &m_upload_semaphores[i]),
                           "Failed to create semaphore");
            //VK_CRASH_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &device_out[i].semaphore),
            //	"Failed to create semaphore");

            VkFenceCreateInfo fence_info = {};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VK_CRASH_CHECK(vkCreateFence(device, &fence_info, nullptr, &m_upload_fences[i]), "Failed to create fence");
            //VK_CRASH_CHECK(vkCreateFence(device, &fence_info, nullptr, &device_out[i].fence), "Failed to create fence");
        }
    }

    void device_memory::terminate(VkDevice device, u8 current_frame) {
        unmap_ranges(device, current_frame);

        for (buffer_pool &pool: vertex_pools) pool.free(<#initializer#>, <#initializer#>, device);
        vertex_pools.clear();
        for (buffer_pool &pool: index_pools) pool.free(<#initializer#>, <#initializer#>, device);
        index_pools.clear();
        for (buffer_pool &pool: uniform_pools) pool.free(<#initializer#>, <#initializer#>, device);
        uniform_pools.clear();
        for (buffer_pool &pool: storage_pools) pool.free(<#initializer#>, <#initializer#>, device);
        storage_pools.clear();
        for (dynamic_buffer_pool &pool: d_vertex_pools) pool.free(<#initializer#>, <#initializer#>, device);
        d_vertex_pools.clear();
        for (dynamic_buffer_pool &pool: d_index_pools) pool.free(<#initializer#>, <#initializer#>, device);
        d_index_pools.clear();
        for (dynamic_buffer_pool &pool: d_uniform_pools) pool.free(<#initializer#>, <#initializer#>, device);
        d_uniform_pools.clear();
        for (dynamic_buffer_pool &pool: d_storage_pools) pool.free(<#initializer#>, <#initializer#>, device);
        d_storage_pools.clear();
        for (texture_pool &pool: texture_pools) pool.free(<#initializer#>, <#initializer#>, device);
        texture_pools.clear();

        for (upload_pool &pool: m_upload_pools) pool.free(<#initializer#>, <#initializer#>, device);
        m_upload_pools.clear();
        for (texture_upload_pool &pool: m_tex_upload_pools) pool.free(<#initializer#>, <#initializer#>, device);
        m_tex_upload_pools.clear();

        for (u32 i = 0; i < max_frames_in_flight; i++) {
            vkDestroySemaphore(device, m_upload_semaphores[i], nullptr);
            vkDestroyFence(device, m_upload_fences[i], nullptr);

            //vkDestroySemaphore(device, device_out[i].semaphore, nullptr);
            //vkDestroyFence(device, device_out[i].fence, nullptr);
        }

        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }

    bool device_memory::upload(VkDevice device, VkQueue transfer_queue, u32 transfer_queue_idx, u8 current_frame) {
        if (!m_uploads_pending)
            return false;

        VkCommandBuffer &cmd_buffer = cmd_buffers[current_frame];

        vkResetCommandBuffer(cmd_buffer, 0);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd_buffer, &begin_info);

        for (upload_pool &pool: m_upload_pools)
            pool.record_and_clear(cmd_buffer);

        for (texture_upload_pool &pool: m_tex_upload_pools)
            pool.record_and_clear_transfer(cmd_buffer, transfer_queue_idx);

        vkEndCommandBuffer(cmd_buffer);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &m_upload_semaphores[current_frame];

        vkResetFences(device, 1, &m_upload_fences[current_frame]);
        vkQueueSubmit(transfer_queue, 1, &submit_info, m_upload_fences[current_frame]);

        m_uploads_pending = false;

        return true;
    }

    void device_memory::map_ranges(VkDevice device, u8 current_frame) {
        for (dynamic_buffer_pool &pool: d_vertex_pools) pool.map(device, current_frame);
        for (dynamic_buffer_pool &pool: d_index_pools) pool.map(device, current_frame);
        for (dynamic_buffer_pool &pool: d_uniform_pools) pool.map(device, current_frame);
        for (dynamic_buffer_pool &pool: d_storage_pools) pool.map(device, current_frame);

        for (upload_pool &pool: m_upload_pools) pool.map(device, current_frame);
        for (texture_upload_pool &pool: m_tex_upload_pools) pool.map(device, current_frame);
    }

    void device_memory::unmap_ranges(VkDevice device, u8 current_frame) {
        for (dynamic_buffer_pool &pool: d_vertex_pools) pool.unmap(device);
        for (dynamic_buffer_pool &pool: d_index_pools) pool.unmap(device);
        for (dynamic_buffer_pool &pool: d_uniform_pools) pool.unmap(device);
        for (dynamic_buffer_pool &pool: d_storage_pools) pool.unmap(device);

        for (upload_pool &pool: m_upload_pools) pool.unmap(device);
        for (texture_upload_pool &pool: m_tex_upload_pools) pool.unmap(device);
    }

    void device_memory::flush_ranges(VkDevice device, u8 current_frame) {
        std::vector<VkMappedMemoryRange> ranges;

        if (!heap_manager.host_coherent_dynamic_heap()) {
            dynamic_buffer_pool::push_flush_ranges(ranges, current_frame, d_vertex_pools);
            dynamic_buffer_pool::push_flush_ranges(ranges, current_frame, d_index_pools);
            dynamic_buffer_pool::push_flush_ranges(ranges, current_frame, d_uniform_pools);
            dynamic_buffer_pool::push_flush_ranges(ranges, current_frame, d_storage_pools);
        }

        if (!heap_manager.host_coherent_upload_heap()) {
            staging_pool::push_flush_ranges(ranges, current_frame, m_upload_pools);
            staging_pool::push_flush_ranges(ranges, current_frame, m_tex_upload_pools);
        }

        if (!ranges.empty())
            VK_CRASH_CHECK(vkFlushMappedMemoryRanges(device, ranges.size(), ranges.data()),
                           "Failed to flush memory ranges");
    }

    void device_memory::sync(VkDevice device, u8 current_frame) {
        vkWaitForFences(device, 1, &m_upload_fences[current_frame], VK_TRUE, std::numeric_limits<u64>::max());
    }

    memory_ref device_memory::alloc_vertices(VkDeviceSize size) { return alloc_buffer<VERTEX, false>(size); }

    memory_ref device_memory::alloc_vertices(const void *data, VkDeviceSize size) {
        return alloc_buffer<VERTEX, false>(data, size);
    }

    memory_ref device_memory::alloc_indexes(VkDeviceSize size) { return alloc_buffer<INDEX, false>(size); }

    memory_ref device_memory::alloc_indexes(const void *data, VkDeviceSize size) {
        return alloc_buffer<INDEX, false>(data, size);
    }

    memory_ref device_memory::alloc_uniform(VkDeviceSize size) { return alloc_buffer<UNIFORM, false>(size); }

    memory_ref device_memory::alloc_uniform(const void *data, VkDeviceSize size) {
        return alloc_buffer<UNIFORM, false>(data, size);
    }

    memory_ref device_memory::alloc_storage(VkDeviceSize size) { return alloc_buffer<STORAGE, false>(size); }

    memory_ref device_memory::alloc_storage(const void *data, VkDeviceSize size) {
        return alloc_buffer<STORAGE, false>(data, size);
    }

    memory_ref device_memory::alloc_dynamic_vertices(VkDeviceSize size) { return alloc_buffer<VERTEX, true>(size); }

    memory_ref device_memory::alloc_dynamic_vertices(const void *data, VkDeviceSize size) {
        return alloc_buffer<VERTEX, true>(data, size);
    }

    memory_ref device_memory::alloc_dynamic_indexes(VkDeviceSize size) { return alloc_buffer<INDEX, true>(size); }

    memory_ref device_memory::alloc_dynamic_indexes(const void *data, VkDeviceSize size) {
        return alloc_buffer<INDEX, true>(data, size);
    }

    memory_ref device_memory::alloc_dynamic_uniform(VkDeviceSize size) { return alloc_buffer<UNIFORM, true>(size); }

    memory_ref device_memory::alloc_dynamic_uniform(const void *data, VkDeviceSize size) {
        return alloc_buffer<UNIFORM, true>(data, size);
    }

    memory_ref device_memory::alloc_dynamic_storage(VkDeviceSize size) { return alloc_buffer<STORAGE, true>(size); }

    memory_ref device_memory::alloc_dynamic_storage(const void *data, VkDeviceSize size) {
        return alloc_buffer<STORAGE, true>(data, size);
    }

    void device_memory::upload_vertices(const memory_ref &ref, const void *data, VkDeviceSize size,
                                        VkDeviceSize offset) { this->submit_upload<VERTEX>(ref, data, size, offset); }

    void device_memory::upload_indexes(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->submit_upload<INDEX>(ref, data, size, offset); }

    void device_memory::upload_uniform(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->submit_upload<UNIFORM>(ref, data, size, offset); }

    void device_memory::upload_storage(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->submit_upload<STORAGE>(ref, data, size, offset); }

    void device_memory::memcpy_vertices(const memory_ref &ref, const void *data, VkDeviceSize size,
                                        VkDeviceSize offset) { this->memcpy<VERTEX>(ref, data, size, offset); }

    void device_memory::memcpy_indexes(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->memcpy<INDEX>(ref, data, size, offset); }

    void device_memory::memcpy_uniform(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->memcpy<UNIFORM>(ref, data, size, offset); }

    void device_memory::memcpy_storage(const memory_ref &ref, const void *data, VkDeviceSize size,
                                       VkDeviceSize offset) { this->memcpy<STORAGE>(ref, data, size, offset); }

    void device_memory::vertex_map(const memory_ref &ref, void **&out_map_ptr,
                                   std::size_t &out_offset) noexcept { map<VERTEX>(ref, out_map_ptr, out_offset); }

    void device_memory::index_map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept {
        map<INDEX>(ref, out_map_ptr, out_offset);
    }

    void device_memory::uniform_map(const memory_ref &ref, void **&out_map_ptr,
                                    std::size_t &out_offset) noexcept { map<UNIFORM>(ref, out_map_ptr, out_offset); }

    void device_memory::storage_map(const memory_ref &ref, void **&out_map_ptr,
                                    std::size_t &out_offset) noexcept { map<STORAGE>(ref, out_map_ptr, out_offset); }

    buffer_binding_args device_memory::binding_args(const mesh &object) noexcept {
        return binding_args<VERTEX>(object);
    }

    buffer_binding_args
    device_memory::binding_args(const uniform &uniform) noexcept { return dynamic_binding_args<UNIFORM>(uniform); }

    buffer_binding_args
    device_memory::binding_args(const unmapped_uniform &uniform) noexcept { return binding_args<UNIFORM>(uniform); }

    buffer_binding_args device_memory::binding_args(const storage_array &array) noexcept {
        return binding_args<STORAGE>(array);
    }

    buffer_binding_args device_memory::binding_args(const dynamic_storage_array &array) noexcept {
        return dynamic_binding_args<STORAGE>(array);
    }

    buffer_binding_args
    device_memory::binding_args(const storage_vector &vector) noexcept { return binding_args<STORAGE>(vector); }

    buffer_binding_args device_memory::binding_args(const dynamic_storage_vector &vector) noexcept {
        return dynamic_binding_args<STORAGE>(vector);
    }

    template<device_memory::buffer_t BType, bool Dynamic>
    inline memory_ref device_memory::alloc_buffer(VkDeviceSize size) {
        using pool_t = typename get_buffer_pool<Dynamic>::type;
        const VkDeviceSize alignment = offset_alignment<BType>();
        std::vector<pool_t> *pools;
        VkDeviceSize pool_size;
        if constexpr (!Dynamic) {
            pools = &static_pools<BType>();
            pool_size = buffer<BType>::size;
        } else {
            pools = &dynamic_pools<BType>();
            pool_size = buffer<BType>::dynamic_size;
        }

        u32 selected_pool_idx = 0;
        u32 selected_slot_idx = 0;
        VkDeviceSize size_needed = 0;
        VkDeviceSize offset = 0;

        if (!search_buffer_pools(*pools, size, alignment, selected_pool_idx, selected_slot_idx, size_needed, offset)) {
            if (pool_size < size)
                pool_size = increase_to_fit(pool_size, size);

            if constexpr (!Dynamic || BType == buffer_t::UNIVERSAL) {
                pools->push_back(buffer_pool(radd.device, heap_manager, pool_size, buffer<BType>::usage));
            } else {
                pools->push_back(
                        dynamic_buffer_pool(radd.device, heap_manager, pool_size, buffer<BType>::dynamic_usage));
                (*pools)[selected_pool_idx].map(radd.device, *radd.current_frame);
            }
        }

        pool_t &pool = (*pools)[selected_pool_idx];
        pool.fill_slot(selected_slot_idx, size_needed);

        LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} (offset {2}->{3}) of pool {4} {5}",
                           size, selected_slot_idx, offset,
                           aligned_offset(offset, alignment), buffer<BType>::debug_name, selected_pool_idx);

        return mem_ref_internal::create(BType, selected_pool_idx, offset, size);
    }

    template<device_memory::buffer_t BType, bool Dynamic>
    inline memory_ref device_memory::alloc_buffer(const void *data, VkDeviceSize size) {
        memory_ref ref = alloc_buffer<BType, Dynamic>(size);

        if constexpr (!Dynamic || BType == buffer_t::UNIVERSAL)
            submit_upload<BType>(ref, data, size, 0);
        else
            this->memcpy<BType>(ref, data, size, 0);

        return ref;
    }

    template<device_memory::buffer_t BType>
    inline void
    device_memory::submit_upload(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset) {
        const internal_ref &iref = mem_ref_internal::extract(ref);
        INTERNAL_ASSERT(iref.pool_type == BType, "Memory reference pool type and function pool type do not match");
        INTERNAL_ASSERT(iref.offset + offset + size <= iref.size, "Out of bounds memory access");

        std::size_t up_pool_idx = 0;
        for (upload_pool &pool: m_upload_pools) {
            if (pool.size() + size <= pool.capacity())
                break;

            up_pool_idx++;
        }

        if (up_pool_idx == m_upload_pools.size()) {
            VkDeviceSize up_pool_size = staging_buffer_size;
            if (up_pool_size < size)
                up_pool_size = increase_to_fit(up_pool_size, size);

            m_upload_pools.push_back(upload_pool(radd.device, heap_manager, up_pool_size));
            m_upload_pools[up_pool_idx].map(radd.device, *radd.current_frame);
        }

        upload_pool &up_pool = m_upload_pools[up_pool_idx];

        if (!up_pool.has_batch(iref.pool_type, iref.pool))
            up_pool.create_batch(iref.pool_type, iref.pool, static_pools<BType>()[iref.pool].buffer());

        up_pool.submit(iref.pool_type, iref.pool, data, size, iref.offset + offset);

        m_uploads_pending = true;
    }

    template<device_memory::buffer_t BType>
    inline void device_memory::memcpy(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset) {
        const internal_ref &iref = mem_ref_internal::extract(ref);
        INTERNAL_ASSERT(iref.pool_type == BType, "Memory reference pool type and function pool type do not match");
        INTERNAL_ASSERT(iref.offset + offset + size <= iref.size, "Out of bounds memory access");
        std::vector<dynamic_buffer_pool> &pools = dynamic_pools<BType>();
        std::memcpy(static_cast<std::byte *>(pools[iref.pool].host_ptr()) + iref.offset + offset, data, size);
    }

    template<device_memory::buffer_t BType>
    void device_memory::map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept {
        const internal_ref &iref = mem_ref_internal::extract(ref);
        out_offset = aligned_offset(iref.offset, offset_alignment<BType>());
        out_map_ptr = &dynamic_pools<BType>()[iref.pool].host_ptr();
    }

    buffer_binding_args device_memory::index_binding_args(const mesh &mesh) noexcept {
        INTERNAL_ASSERT(mesh.valid(), "Invalid mesh");
        const internal_ref &ref = mesh_internal::extract_index_ref(mesh);
        return {index_pools[ref.pool].buffer(), index_pools[ref.pool].size(), ref.offset, ref.size};
    }

    template<device_memory::buffer_t BType>
    buffer_binding_args device_memory::binding_args(const resource &resource) noexcept {
        INTERNAL_ASSERT(resource.valid(), "Invalid resource");
        const internal_ref &ref = resource_internal::extract_ref(resource);
        return {static_pools<BType>()[ref.pool].buffer(), 0, aligned_offset(ref.offset, offset_alignment<BType>()),
                ref.size};
    }

    template<device_memory::buffer_t BType>
    buffer_binding_args device_memory::dynamic_binding_args(const resource &resource) noexcept {
        INTERNAL_ASSERT(resource.valid(), "Invalid resource");
        const internal_ref &ref = resource_internal::extract_ref(resource);
        dynamic_buffer_pool &pool = dynamic_pools<BType>()[ref.pool];
        return {pool.buffer(), pool.size(), aligned_offset(ref.offset, offset_alignment<BType>()), ref.size};
    }

    template<bool Dynamic>
    inline memory_ref device_memory::alloc_texture(VkExtent3D extent, u32 layers, u32 mip_levels) {
        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.flags = 0; //TODO extend depending on type of image

        image_info.imageType =
                extent.depth == 1 ? (extent.height == 1 ? VK_IMAGE_TYPE_1D : VK_IMAGE_TYPE_2D) : VK_IMAGE_TYPE_3D;

        image_info.format = VK_FORMAT_R8G8B8A8_SRGB; //TODO vkCmdBlitImage to submit and change image format
        image_info.extent = extent;
        image_info.mipLevels = mip_levels;
        image_info.arrayLayers = layers;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT; //TODO extend if image is a render target

        image_info.tiling = Dynamic ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

        image_info.usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //TODO extend depending on image
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;

        image_info.initialLayout = Dynamic ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

        VkMemoryRequirements requirements;
        texture_slot tex = create_texture(radd.device, image_info, requirements);

        using pool_t = typename get_texture_pool<Dynamic>::type;
        std::vector<pool_t> *pools;
        VkDeviceSize pool_size;
        if constexpr (!Dynamic) {
            pools = &texture_pools;
            pool_size = buffer<TEXTURE>::size;
        } else {
            static_assert(force_eval<Dynamic>::value, "Unimplemented");
            //pools = &dynamic_pools<BType>();
            pool_size = buffer<TEXTURE>::dynamic_size;
        }

        u32 selected_pool_idx = 0;
        u32 selected_slot_idx = 0;
        VkDeviceSize size_needed = 0;
        VkDeviceSize offset = 0;

        if (!search_texture_pools(*pools, requirements, selected_pool_idx, selected_slot_idx, size_needed, offset)) {
            if (pool_size < requirements.size)
                pool_size = increase_to_fit(pool_size, requirements.size);

            if constexpr (!Dynamic) {
                pools->push_back(texture_pool(radd.device, heap_manager, pool_size,
                                              requirements.memoryTypeBits, device_heap_manager::heap::MAIN));
            } else {
                //pools->push_back(dynamic_buffer_pool(radd.device, heap_manager, pool_size,
                //	buffer<BType>::dynamic_usage, host_visible_heap));
                //pools->at(selected_pool_idx).map(radd.device, *radd.current_frame);
            }
        }

        pool_t &pool = (*pools)[selected_pool_idx];
        pool.fill_slot(radd.device, std::move(tex), selected_slot_idx, size_needed, requirements.alignment);

        LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} (offset {2}->{3}) of pool {4} {5}",
                           requirements.size, selected_slot_idx, offset,
                           aligned_offset(offset, requirements.alignment), buffer<TEXTURE>::debug_name,
                           selected_pool_idx);

        return mem_ref_internal::create(TEXTURE, selected_pool_idx, offset, requirements.size);
    }

    void device_memory::submit_texture_upload(const memory_ref &ref, const void *data) {
        const internal_ref &iref = mem_ref_internal::extract(ref);
        INTERNAL_ASSERT(iref.pool_type == TEXTURE, "Memory reference pool type and function pool type do not match");

        VkDeviceSize size = iref.size;
        std::size_t up_pool_idx = 0;
        for (texture_upload_pool &pool: m_tex_upload_pools) {
            if (pool.size() + size <= pool.capacity())
                break;

            up_pool_idx++;
        }

        if (up_pool_idx == m_tex_upload_pools.size()) {
            VkDeviceSize up_pool_size = staging_buffer_size;
            if (up_pool_size < size)
                up_pool_size = increase_to_fit(up_pool_size, size);

            m_tex_upload_pools.emplace_back(radd.device, heap_manager, up_pool_size);
            m_tex_upload_pools[up_pool_idx].map(radd.device, *radd.current_frame);
        }

        texture_upload_pool &up_pool = m_tex_upload_pools[up_pool_idx];

        const texture_pool &tex_pool = texture_pools[iref.pool];
        const texture_slot &tex = tex_pool.tex_at(tex_pool.find_slot(iref.offset));
        up_pool.buffer_image_copy(data, size, tex.image, VK_IMAGE_LAYOUT_GENERAL, tex.dims);

        m_uploads_pending = true;
    }

    memory_ref device_memory::alloc_texture(u32 w, u32 h) {
        return alloc_texture<false>({w, h, 1U}, 1, 1);
    }
}
*/

namespace hc::render::device {
    Result<Memory, MemoryResult> Memory::create(VkPhysicalDevice physical_device, const VolkDeviceTable &fn_table,
                                                VkDevice device, const VkPhysicalDeviceLimits &limits) {
        auto manager = memory::HeapManager::create(physical_device);
        if (!manager)
            return Err(MemoryResult::HeapError);

        Memory memory;
        memory.limits = limits;
        memory.heap_manager = std::move(manager).ok();
        return Ok(std::move(memory));
    }

    Memory::~Memory() {
        HC_ASSERT(this->buffer_pools.empty(), "Memory must be externally freed using `destroy` before the destruction");
        HC_ASSERT(this->dynamic_buffer_pools.empty(),
                  "Memory must be externally freed using `destroy` before the destruction");
    }

    void Memory::destroy(const VolkDeviceTable &fn_table, VkDevice device) {
        for (auto &[flags, pools]: this->buffer_pools) {
            for (auto &[id, pool]: pools) {
                pool.free(fn_table, device, this->heap_manager);
            }
        }
        this->buffer_pools.clear();

        for (auto &[flags, pools]: this->dynamic_buffer_pools) {
            for (auto &[id, pool]: pools) {
                pool.free(fn_table, device, this->heap_manager);
            }
        }
        this->dynamic_buffer_pools.clear();
    }

    MemoryResult Memory::map_ranges(const VolkDeviceTable &fn_table, VkDevice device, u8 frame_mod) {
        for (auto &[flags, pools]: this->dynamic_buffer_pools) {
            for (auto &[id, pool]: pools) {
                memory::PoolResult res = pool.map(fn_table, device, frame_mod);
                if (res != memory::PoolResult::Success)
                    return MemoryResult::MapError; // TODO might want to clean up already mapped ranges
            }
        }

        return MemoryResult::Success;
    }

    void Memory::unmap_ranges(const VolkDeviceTable &fn_table, VkDevice device) {
        for (auto &[flags, pools]: this->dynamic_buffer_pools) {
            for (auto &[id, pool]: pools) {
                pool.unmap(fn_table, device);
            }
        }
    }

    MemoryResult Memory::flush_ranges(const VolkDeviceTable &fn_table, VkDevice device, u8 frame_mod) {
        std::vector<VkMappedMemoryRange> ranges;

        if (!this->heap_manager.host_coherent_dynamic_heap()) {
            for (auto &[flags, pools]: this->dynamic_buffer_pools) {
                for (auto &[id, pool]: pools) {
                    ranges.push_back(pool.mapped_range(frame_mod));
                }
            }
        }

//        if (!heap_manager.host_coherent_upload_heap()) {
//            staging_pool::push_flush_ranges(ranges, current_frame, m_upload_pools);
//            staging_pool::push_flush_ranges(ranges, current_frame, m_tex_upload_pools);
//        }

        if (!ranges.empty()) {
            VkResult res = fn_table.vkFlushMappedMemoryRanges(device, ranges.size(), ranges.data());
            switch (res) {
                case VK_SUCCESS:
                    // Nothing, keep going
                    break;
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    return MemoryResult::FlushError;
                default: HC_UNREACHABLE("vkFlushMappedMemoryRanges should not return any other VkResult values");
            }
        }

        return MemoryResult::Success;
    }

    inline VkDeviceSize increase_to_fit(VkDeviceSize base, VkDeviceSize target) {
        if (target <= base)
            return base;

        // Mathematical equivalent to a loop doubling base until target is smaller
        VkDeviceSize exp = std::ceil(std::log2(static_cast<double>(target) / static_cast<double>(base)));
        return base * (VkDeviceSize(1) << exp);
    }

    VkDeviceSize Memory::alignment_of(VkBufferUsageFlags flags) const noexcept {
        if (flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            return this->limits.minStorageBufferOffsetAlignment;

        if (flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            return this->limits.minUniformBufferOffsetAlignment;

        // Using 64 as the default should prevent any unspecified alignment issues (vertexes/indexes) and may even help
        // with cache usage
        return 64;
    }

    Result<memory::Ref, MemoryResult> Memory::alloc(const VolkDeviceTable &fn_table, VkDevice device,
                                                    VkBufferUsageFlags flags, VkDeviceSize size) {
        if (!this->buffer_pools.contains(flags))
            this->buffer_pools.insert({flags, {}});

        VkDeviceSize alignment = this->alignment_of(flags);
        auto &pools = this->buffer_pools[flags];
        u64 pool_id = 0;
        memory::AllocationSpec spec = {};
        for (const auto &[id, pool]: pools) {
            spec = pool.search(size, alignment);
            if (spec.size) {
                pool_id = id;
                break;
            }
        }

        if (!spec.size) {
            VkDeviceSize pool_size = increase_to_fit(MEBI(8), size);
            auto pool_result = memory::BufferPool::create(fn_table, device, this->heap_manager, pool_size, flags);
            if (pool_result) {
                pool_id = pools.insert(std::move(pool_result).ok());
                spec = pools[pool_id].search(size, alignment);
                HC_ASSERT(spec.size, "Search must succeed here");
            } else {
                memory::PoolResult err = pool_result.err();
                switch (err) {
                    case memory::PoolResult::OutOfDeviceMemory:
                        return Err(MemoryResult::OutOfHostMemory);
                    case memory::PoolResult::OutOfHostMemory:
                        return Err(MemoryResult::OutOfDeviceMemory);
                    case memory::PoolResult::UnsupportedHeap:
                        return Err(MemoryResult::HeapError);
                    default: HC_UNREACHABLE("BufferPool::create should not return any other values");
                }
            }
        }

        HC_TRACE("Allocating in pool " << pool_id << ':' << flags << ", at " << spec.offset << " bytes offset, "
                                       << spec.size << " bytes + " << spec.padding << " padding bytes");
        pools[pool_id].fill_slot(spec.slot_idx, spec.size + spec.padding);

        return Ok(memory::Ref{
                .buffer = pools[pool_id].handle(),
                .pool = pool_id,
                .pool_size = pools[pool_id].capacity(),
                .size = spec.size,
                .offset = spec.offset,
                .padding = spec.padding,
                .flags = flags,
        });
    }

    Result<std::pair<memory::Ref, void **>, MemoryResult> Memory::alloc_dyn(const VolkDeviceTable &fn_table,
                                                                            VkDevice device, VkBufferUsageFlags flags,
                                                                            VkDeviceSize size, u8 frame_mod) {
        if (!this->dynamic_buffer_pools.contains(flags))
            this->dynamic_buffer_pools.insert({flags, {}});

        VkDeviceSize alignment = this->alignment_of(flags);
        auto &pools = this->dynamic_buffer_pools[flags];
        u32 pool_id = 0;
        memory::AllocationSpec spec = {};
        for (const auto &[id, pool]: pools) {
            spec = pool.search(size, alignment);
            if (spec.size) {
                pool_id = id;
                break;
            }
        }

        if (!spec.size) {
            VkDeviceSize pool_size = increase_to_fit(MEBI(8), size);
            auto pool_result = memory::DynamicBufferPool::create(fn_table, device, this->heap_manager, pool_size,
                                                                 flags);
            if (pool_result) {
                pool_id = pools.insert(std::move(pool_result).ok());

                memory::PoolResult res = pools[pool_id].map(fn_table, device, frame_mod);
                if (res != memory::PoolResult::Success) {
                    pools[pool_id].free(fn_table, device, this->heap_manager);
                    pools.erase(pool_id);
                    return Err(MemoryResult::MapError);
                }

                spec = pools[pool_id].search(size, alignment);
                HC_ASSERT(spec.size, "Search must succeed here");
            } else {
                memory::PoolResult err = pool_result.err();
                switch (err) {
                    case memory::PoolResult::OutOfDeviceMemory:
                        return Err(MemoryResult::OutOfHostMemory);
                    case memory::PoolResult::OutOfHostMemory:
                        return Err(MemoryResult::OutOfDeviceMemory);
                    case memory::PoolResult::UnsupportedHeap:
                        return Err(MemoryResult::HeapError);
                    default: HC_UNREACHABLE("DynamicBufferPool::create should not return any other values");
                }
            }
        }

        HC_TRACE("Allocating in dynamic pool " << pool_id << ':' << flags << ", at " << spec.offset << " bytes offset, "
                                               << spec.size << " bytes + " << spec.padding << " padding bytes");
        pools[pool_id].fill_slot(spec.slot_idx, spec.size + spec.padding);
        memory::Ref ref = {
                .buffer = pools[pool_id].handle(),
                .pool = pool_id,
                .pool_size = pools[pool_id].capacity(),
                .size = spec.size,
                .offset = spec.offset,
                .padding = spec.padding,
                .flags = flags,
        };
        void **host_ptr = pools[pool_id].host_ptr();

        return Ok(std::make_pair(ref, host_ptr));
    }

    void Memory::free(const VolkDeviceTable &fn_table, VkDevice device, ResourceDestructionMark mark) {
        if (mark.dynamic) {
            HC_ASSERT(this->dynamic_buffer_pools.contains(mark.usage), "Pool list matching the flags must exist");
            HC_TRACE("Freeing in dynamic pool " << mark.pool << ':' << mark.usage << " at offset " << mark.offset);
            this->dynamic_buffer_pools[mark.usage][mark.pool].clear_slot(mark.offset);
        } else {
            HC_ASSERT(this->buffer_pools.contains(mark.usage), "Pool list matching the flags must exist");
            HC_TRACE("Freeing in pool " << mark.pool << ':' << mark.usage << " at offset " << mark.offset);
            this->buffer_pools[mark.usage][mark.pool].clear_slot(mark.offset);
        }
    }
}
