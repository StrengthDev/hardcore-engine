#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "shader_internal.hpp"
#include "call_instances.hpp"
#include "memory.hpp"
#include <render/resource.hpp>

#include <core/exception.hpp>

#include <vector>
#include <unordered_map>

namespace ENGINE_NAMESPACE {
    class device;

    class graphics_pipeline {
    public:
        graphics_pipeline(device &owner, const std::vector<const shader *> &shaders);

        graphics_pipeline(device &owner, const std::vector<const shader *> &shaders, const VkExtent2D &extent);

        ~graphics_pipeline();

        graphics_pipeline(const graphics_pipeline &) = delete;

        inline graphics_pipeline &operator=(const graphics_pipeline &) = delete;

        graphics_pipeline(graphics_pipeline &&other) noexcept: owner(other.owner),
                                                               n_descriptor_set_layouts(
                                                                       std::exchange(other.n_descriptor_set_layouts,
                                                                                     0)),
                                                               descriptor_set_layouts(
                                                                       std::exchange(other.descriptor_set_layouts,
                                                                                     VK_NULL_HANDLE)),
                                                               pipeline_layout(std::exchange(other.pipeline_layout,
                                                                                             VK_NULL_HANDLE)),
                                                               handle(std::exchange(other.handle, VK_NULL_HANDLE)),
                                                               n_descriptor_pool_sizes{
                                                                       std::exchange(other.n_descriptor_pool_sizes[0],
                                                                                     0),
                                                                       std::exchange(other.n_descriptor_pool_sizes[1],
                                                                                     0)},
                                                               descriptor_pool_sizes{
                                                                       std::exchange(other.descriptor_pool_sizes[0],
                                                                                     nullptr),
                                                                       std::exchange(other.descriptor_pool_sizes[1],
                                                                                     nullptr)},
                                                               descriptor_sets(
                                                                       std::exchange(other.descriptor_sets, nullptr)),
                                                               object_descriptor_set_capacity(std::exchange(
                                                                       other.object_descriptor_set_capacity, 0)),
                                                               frame_descriptors(std::move(other.frame_descriptors)),
                                                               frame_descriptor_count(
                                                                       std::exchange(other.frame_descriptor_count, 0)),
                                                               n_object_bindings(
                                                                       std::exchange(other.n_object_bindings, 0)),
                                                               object_binding_types(
                                                                       std::exchange(other.object_binding_types,
                                                                                     nullptr)),
                                                               n_dynamic_descriptors(
                                                                       std::exchange(other.n_dynamic_descriptors, 0)),
                                                               object_refs(std::move(other.object_refs)),
                                                               objects(std::move(other.objects)),
                                                               cached_object_bindings(
                                                                       std::move(other.cached_object_bindings)) {}

        inline graphics_pipeline &operator=(graphics_pipeline &&other) noexcept {
            this->~graphics_pipeline();
            owner = other.owner;
            n_descriptor_set_layouts = std::exchange(other.n_descriptor_set_layouts, 0);
            descriptor_set_layouts = std::exchange(other.descriptor_set_layouts, VK_NULL_HANDLE);
            pipeline_layout = std::exchange(other.pipeline_layout, VK_NULL_HANDLE);
            handle = std::exchange(other.handle, VK_NULL_HANDLE);
            n_descriptor_pool_sizes[0] = std::exchange(other.n_descriptor_pool_sizes[0], 0);
            n_descriptor_pool_sizes[1] = std::exchange(other.n_descriptor_pool_sizes[1], 0);
            descriptor_pool_sizes[0] = std::exchange(other.descriptor_pool_sizes[0], nullptr);
            descriptor_pool_sizes[1] = std::exchange(other.descriptor_pool_sizes[1], nullptr);
            descriptor_sets = std::exchange(other.descriptor_sets, nullptr);
            object_descriptor_set_capacity = std::exchange(other.object_descriptor_set_capacity, 0);
            frame_descriptors = std::move(other.frame_descriptors);
            frame_descriptor_count = std::exchange(other.frame_descriptor_count, 0);
            n_object_bindings = std::exchange(other.n_object_bindings, 0);
            object_binding_types = std::exchange(other.object_binding_types, nullptr);
            n_dynamic_descriptors = std::exchange(other.n_dynamic_descriptors, 0);
            object_refs = std::move(other.object_refs);
            objects = std::move(other.objects);
            cached_object_bindings = std::move(other.cached_object_bindings);
            return *this;
        }

        void record_commands(VkCommandBuffer &buffer, u8 current_frame);

        void update_descriptor_sets(u8 previous_frame, u8 current_frame, u8 next_frame);

        std::size_t add(const mesh &object);

        void remove(const mesh &object);

        std::size_t set_instances(const mesh &object, u32 num);

        inline void set_instances(std::size_t object_idx, u32 num) { objects[object_idx].properties().instances = num; }

        template<typename Type,
                std::enable_if_t<std::is_base_of<resource, Type>::value && std::is_final<Type>::value, bool> = true>
        inline std::size_t set_descriptor(const mesh &object, u32 descriptor_idx, const Type &buffer) {
            auto it = object_refs.find(std::hash<resource>{}(object));
            if (it == object_refs.end())
                throw std::range_error("Specified mesh not found in pipeline");
            std::size_t idx = it->second;
            set_descriptor(idx, descriptor_idx, buffer);
            return idx;
        }

        template<typename Type,
                std::enable_if_t<std::is_base_of<resource, Type>::value && std::is_final<Type>::value, bool> = true>
        inline void set_descriptor(std::size_t object_idx, u32 descriptor_idx, const Type &buffer) {
            INTERNAL_ASSERT(n_object_bindings > descriptor_idx, "Descriptor index out of bounds");
            if (!type_match(descriptor_idx, buffer)) {
                std::stringstream stream;
                stream << "Incorrect descriptor type for binding: " << descriptor_idx << " (correct type is: " <<
                       debug_descriptor_type(object_binding_types[descriptor_idx]) << ')';
                throw exception::type_mismatch(stream.str());
            }
            set_descriptor(object_idx, descriptor_idx, get_binding(buffer));
        }

    private:
        graphics_pipeline(device &owner, const std::vector<const shader *> &shaders, const VkExtent2D &extent,
                          bool dynamic_viewport);

        inline bool type_match(u32 descriptor_idx, const uniform &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }

        inline bool type_match(u32 descriptor_idx, const unmapped_uniform &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }

        inline bool type_match(u32 descriptor_idx, const storage_array &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }

        inline bool type_match(u32 descriptor_idx, const dynamic_storage_array &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }

        inline bool type_match(u32 descriptor_idx, const storage_vector &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }

        inline bool type_match(u32 descriptor_idx, const dynamic_storage_vector &) const noexcept {
            return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }

        buffer_binding_args get_binding(const uniform &resource) const;

        buffer_binding_args get_binding(const unmapped_uniform &resource) const;

        buffer_binding_args get_binding(const storage_array &resource) const;

        buffer_binding_args get_binding(const dynamic_storage_array &resource) const;

        buffer_binding_args get_binding(const storage_vector &resource) const;

        buffer_binding_args get_binding(const dynamic_storage_vector &resource) const;

        void set_descriptor(std::size_t object_idx, u32 descriptor_idx, buffer_binding_args &&binding);

        device *owner;

        u32 n_descriptor_set_layouts = 0;
        VkDescriptorSetLayout *descriptor_set_layouts = nullptr;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkPipeline handle = VK_NULL_HANDLE;

        u32 push_data_size = 0;
        VkShaderStageFlags push_flags = 0;

        //object descriptors => set 0 ; pipeline descriptors => set 1
        u32 n_descriptor_pool_sizes[2] = {0, 0}; //TODO change
        VkDescriptorPoolSize *descriptor_pool_sizes[2] = {nullptr, nullptr};

        VkDescriptorSet *descriptor_sets = nullptr;
        u32 object_descriptor_set_capacity = 0;

        struct frame_descriptor_data {
            VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
            u32 object_set_cap = 0;
            // Client made changes this frame
            bool dirty = false;
            // No changes this frame, but in previous frame
            bool outdated = false;
        };

        std::array<frame_descriptor_data, max_frames_in_flight> frame_descriptors;

        u32 frame_descriptor_count = 0;

        u32 n_object_bindings = 0;
        VkDescriptorType *object_binding_types = nullptr;
        u32 n_dynamic_descriptors = 0;


        using task_properties = object_vector::properties_t;
        typedef object_vector::offset_t dynamic_offset_t;

        std::unordered_map<std::size_t, std::size_t> object_refs; //TODO change key to memory_ref to avoid collisions
        object_vector objects;

        std::vector<buffer_binding_args> cached_object_bindings;
        std::vector<VkDescriptorBufferInfo> cached_buffer_infos;

        static void draw_object(VkCommandBuffer &buffer, const task_properties &obj);

        std::vector<VkWriteDescriptorSet> generate_descriptor_write(u8 current_frame);

        inline static const char *debug_descriptor_type(VkDescriptorType type) {
            switch (type) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                    return "Sampler";
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    return "Image sampler";
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    return "Sampled image";
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    return "Storage image";
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    return "Texel uniform";
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    return "Texel storage";
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    return "Uniform";
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    return "Storage";
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    return "Dynamic uniform";
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    return "Dynamic storage";
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    return "Input attachment";
                case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
                    return "Inline uniform";
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    return "Acceleration structure";
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
                    return "Acceleration structure NV";
                case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:
                    return "Mutable valve";
                case VK_DESCRIPTOR_TYPE_MAX_ENUM:
                    return "MAX ENUM";
                default:
                    break;
            }
            return "INVALID TYPE";
        }
    };
}
