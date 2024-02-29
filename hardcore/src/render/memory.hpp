#pragma once

#include "render_core.hpp"
#include "device_heap_manager.hpp"
#include "resource_pool.hpp"
#include "staging_pool.hpp"

#include <render/memory_reference.hpp>
#include <render/resource.hpp>

namespace ENGINE_NAMESPACE {
    // for operations such as allocations, which happen outside of device functions, memory needs access to some device
    // data, such as the VkDevice handle for operations like vkAllocateMemory
    struct random_access_device_data {
        VkDevice device; // the device handle should never change throughout memory's lifetime, so it can be a local copy
        const VkPhysicalDeviceLimits *limits;
        const u8 *current_frame;
    };

    struct buffer_binding_args {
        VkBuffer buffer;
        VkDeviceSize frame_offset;
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    class device_memory {
    public:
        void init(VkPhysicalDevice physical_device, VkDevice device, u32 transfer_queue_idx,
                  const VkPhysicalDeviceLimits *limits, const u8 *current_frame);

        void terminate(VkDevice device, u8 current_frame);

        device_memory() = default;

        device_memory(const device_memory &) = delete;

        device_memory &operator=(const device_memory &) = delete;

        device_memory(device_memory &&other) noexcept:
                radd(std::exchange(other.radd, {})),
                heap_manager(std::move(other.heap_manager)),
                cmd_pool(std::exchange(other.cmd_pool, VK_NULL_HANDLE)), cmd_buffers(std::move(other.cmd_buffers)),
                vertex_pools(std::move(other.vertex_pools)), index_pools(std::move(other.index_pools)),
                uniform_pools(std::move(other.uniform_pools)), storage_pools(std::move(other.storage_pools)),
                d_vertex_pools(std::move(other.d_vertex_pools)), d_index_pools(std::move(other.d_index_pools)),
                d_uniform_pools(std::move(other.d_uniform_pools)), d_storage_pools(std::move(other.d_storage_pools)),
                m_upload_semaphores(std::move(other.m_upload_semaphores)),
                m_upload_fences(std::move(other.m_upload_fences)),
                m_upload_pools(std::move(other.m_upload_pools)),
                m_tex_upload_pools(std::move(other.m_tex_upload_pools)),
                m_uploads_pending(std::exchange(other.m_uploads_pending, false)) {}

        inline void update_refs(VkDevice device, const VkPhysicalDeviceLimits *limits,
                                const u8 *current_frame) noexcept { radd.device = device, radd.limits = limits, radd.current_frame = current_frame; }

        //void update_largest_slots();
        //void tick(); could maybe replace the above function and performa all updates and cleanup

        bool upload(VkDevice device, VkQueue transfer_queue, u32 transfer_queue_idx, u8 current_frame);
        //void flush_downloads();

        void map_ranges(VkDevice device, u8 current_frame);

        void unmap_ranges(VkDevice device, u8 current_frame);

        void flush_ranges(VkDevice device, u8 current_frame);

        void sync(VkDevice device, u8 current_frame);

        inline VkSemaphore upload_semaphore(u8 current_frame) { return m_upload_semaphores[current_frame]; }

        memory_ref alloc_vertices(VkDeviceSize size);

        memory_ref alloc_vertices(const void *data, VkDeviceSize size);

        memory_ref alloc_indexes(VkDeviceSize size);

        memory_ref alloc_indexes(const void *data, VkDeviceSize size);

        memory_ref alloc_uniform(VkDeviceSize size);

        memory_ref alloc_uniform(const void *data, VkDeviceSize size);

        memory_ref alloc_storage(VkDeviceSize size);

        memory_ref alloc_storage(const void *data, VkDeviceSize size);

        memory_ref alloc_dynamic_vertices(VkDeviceSize size);

        memory_ref alloc_dynamic_vertices(const void *data, VkDeviceSize size);

        memory_ref alloc_dynamic_indexes(VkDeviceSize size);

        memory_ref alloc_dynamic_indexes(const void *data, VkDeviceSize size);

        memory_ref alloc_dynamic_uniform(VkDeviceSize size);

        memory_ref alloc_dynamic_uniform(const void *data, VkDeviceSize size);

        memory_ref alloc_dynamic_storage(VkDeviceSize size);

        memory_ref alloc_dynamic_storage(const void *data, VkDeviceSize size);

        memory_ref alloc_texture(u32 w, u32 h);

        void upload_texture(const memory_ref &ref, const void *data) {
            submit_texture_upload(ref, data);
        }

        void upload_vertices(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void upload_indexes(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void upload_uniform(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void upload_storage(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void memcpy_vertices(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void memcpy_indexes(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void memcpy_uniform(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void memcpy_storage(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

        void vertex_map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept;

        void index_map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept;

        void uniform_map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept;

        void storage_map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept;

        buffer_binding_args binding_args(const mesh &object) noexcept;

        buffer_binding_args index_binding_args(const mesh &object) noexcept;

        buffer_binding_args binding_args(const uniform &uniform) noexcept;

        buffer_binding_args binding_args(const unmapped_uniform &uniform) noexcept;

        buffer_binding_args binding_args(const storage_array &array) noexcept;

        buffer_binding_args binding_args(const dynamic_storage_array &array) noexcept;

        buffer_binding_args binding_args(const storage_vector &vector) noexcept;

        buffer_binding_args binding_args(const dynamic_storage_vector &vector) noexcept;

        enum buffer_t : u8 {
            VERTEX = 0,
            INDEX,
            UNIFORM,
            STORAGE,
            UNIVERSAL, //writable
            TEXTURE,

            ENUM_COUNT,
        };

    private:
        //there are some template functions used by other member functions,
        //they are private so it's not a problem that they aren't defined in the header

        template<buffer_t BType>
        inline std::vector<buffer_pool> &static_pools() {
            static_assert(force_eval<BType>::value, "Unimplemented buffer type");
        }

        template<>
        inline std::vector<buffer_pool> &static_pools<VERTEX>() noexcept { return vertex_pools; }

        template<>
        inline std::vector<buffer_pool> &static_pools<INDEX>() noexcept { return index_pools; }

        template<>
        inline std::vector<buffer_pool> &static_pools<UNIFORM>() noexcept { return uniform_pools; }

        template<>
        inline std::vector<buffer_pool> &static_pools<STORAGE>() noexcept { return storage_pools; }

        template<>
        inline std::vector<buffer_pool> &static_pools<UNIVERSAL>() noexcept { return writable_pools; }

        template<buffer_t BType>
        inline std::vector<dynamic_buffer_pool> &dynamic_pools() {
            static_assert(force_eval<BType>::value, "Unimplemented buffer type");
        }

        template<>
        inline std::vector<dynamic_buffer_pool> &dynamic_pools<VERTEX>() noexcept { return d_vertex_pools; }

        template<>
        inline std::vector<dynamic_buffer_pool> &dynamic_pools<INDEX>() noexcept { return d_index_pools; }

        template<>
        inline std::vector<dynamic_buffer_pool> &dynamic_pools<UNIFORM>() noexcept { return d_uniform_pools; }

        template<>
        inline std::vector<dynamic_buffer_pool> &dynamic_pools<STORAGE>() noexcept { return d_storage_pools; }

        template<buffer_t BType>
        inline VkDeviceSize offset_alignment() const noexcept { return 0; }

        template<>
        inline VkDeviceSize
        offset_alignment<device_memory::UNIFORM>() const noexcept { return radd.limits->minUniformBufferOffsetAlignment; }

        template<>
        inline VkDeviceSize
        offset_alignment<device_memory::STORAGE>() const noexcept { return radd.limits->minStorageBufferOffsetAlignment; }

        template<>
        inline VkDeviceSize
        offset_alignment<device_memory::UNIVERSAL>() const noexcept { return radd.limits->minStorageBufferOffsetAlignment; }

        template<buffer_t BType, bool Dynamic>
        memory_ref alloc_buffer(VkDeviceSize size);

        template<buffer_t BType, bool Dynamic>
        memory_ref alloc_buffer(const void *data, VkDeviceSize size);

        template<buffer_t BType>
        void submit_upload(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset);

        template<buffer_t BType>
        void memcpy(const memory_ref &ref, const void *data, VkDeviceSize size, VkDeviceSize offset);

        template<buffer_t BType>
        void map(const memory_ref &ref, void **&out_map_ptr, std::size_t &out_offset) noexcept;

        template<buffer_t BType>
        buffer_binding_args binding_args(const resource &) noexcept;

        template<buffer_t BType>
        buffer_binding_args dynamic_binding_args(const resource &) noexcept;

        template<bool Dynamic>
        memory_ref alloc_texture(VkExtent3D extent, u32 layers, u32 mip_levels);

        void submit_texture_upload(const memory_ref &ref, const void *data);

        random_access_device_data radd;

        device_heap_manager heap_manager;

        VkCommandPool cmd_pool = VK_NULL_HANDLE;
        std::array<VkCommandBuffer, max_frames_in_flight> cmd_buffers;

        // "read-only" resources

        std::vector<buffer_pool> vertex_pools;
        std::vector<buffer_pool> index_pools;
        std::vector<buffer_pool> uniform_pools;
        std::vector<buffer_pool> storage_pools;

        // "writable" resources (for a resource to be writable in shaders, it has to be a storage buffer,
        // which I am assuming is the slowest type of buffer, so instead of having a variable for each type
        // of buffer, there is only one tagged with all types)

        std::vector<buffer_pool> writable_pools;

        // resources directly writable by the cpu

        std::vector<dynamic_buffer_pool> d_vertex_pools;
        std::vector<dynamic_buffer_pool> d_index_pools;
        std::vector<dynamic_buffer_pool> d_uniform_pools;
        std::vector<dynamic_buffer_pool> d_storage_pools;

        std::vector<texture_pool> texture_pools;

        std::array<VkSemaphore, max_frames_in_flight> m_upload_semaphores;
        std::array<VkFence, max_frames_in_flight> m_upload_fences;
        std::vector<upload_pool> m_upload_pools;
        std::vector<texture_upload_pool> m_tex_upload_pools;
        bool m_uploads_pending = false;
    };
}
