#pragma once

#include "memory.hpp"

#include <core/core.hpp>

namespace ENGINE_NAMESPACE {

    class object_vector {
    public:
        object_vector() = default;

        object_vector(std::size_t push_data_size, u32 n_descriptors, u32 n_dynamic_descriptors);

        ~object_vector();

        object_vector(object_vector &&other) noexcept:
                count(std::exchange(other.count, 0)),
                capacity(std::exchange(other.capacity, 0)),
                push_data_size(std::exchange(other.push_data_size, 0)),
                n_descriptors(std::exchange(other.n_descriptors, 0)),
                n_dynamic_descriptors(std::exchange(other.n_dynamic_descriptors, 0)),
                element_stride(std::exchange(other.element_stride, 0)),
                descriptor_stride(std::exchange(other.descriptor_stride, 0)),
                object_data(std::exchange(other.object_data, nullptr)),
                object_descriptor_data(std::exchange(other.object_descriptor_data, nullptr)),
                smp(std::exchange(other.smp, nullptr)) {}

        inline object_vector &operator=(object_vector &&other) noexcept {
            this->~object_vector();
            count = std::exchange(other.count, 0);
            capacity = std::exchange(other.capacity, 0);
            push_data_size = std::exchange(other.push_data_size, 0);
            n_descriptors = std::exchange(other.n_descriptors, 0);
            n_dynamic_descriptors = std::exchange(other.n_dynamic_descriptors, 0);
            element_stride = std::exchange(other.element_stride, 0);
            descriptor_stride = std::exchange(other.descriptor_stride, 0);
            object_data = std::exchange(other.object_data, nullptr);
            object_descriptor_data = std::exchange(other.object_descriptor_data, nullptr);
            smp = std::exchange(other.smp, nullptr);
            return *this;
        }

        object_vector(const object_vector &) = delete;

        object_vector &operator=(const object_vector &) = delete;

        void remove(std::size_t idx);

        void swap(std::size_t a, std::size_t b);

        inline std::size_t size() const noexcept { return count; }

        struct base_object {
            //void* push_data = nullptr;

            //u32* dynamic_offsets = nullptr;
            //buffer_binding_args* descriptor_args = nullptr;

            std::size_t ref_key = 0;
            buffer_binding_args binding = {};
            u32 count = 0;
            VkIndexType index_t = VK_INDEX_TYPE_NONE_KHR;
            buffer_binding_args index_binding = {};
            u32 descriptor_set_idx = 0;
            u32 instances = 1;
        };

        using properties_t = base_object;
        typedef u32 offset_t;

        class iterator;

        class object_ref {
        public:
            inline properties_t &properties() const noexcept {
                return *reinterpret_cast<base_object *>(static_cast<std::byte *>(data_ptr) + push_data_size +
                                                        offsets_size);
            }

            inline offset_t *dynamic_offsets() const noexcept {
                return reinterpret_cast<offset_t *>(static_cast<std::byte *>(data_ptr) + push_data_size);
            }

            inline void *push_data() const noexcept { return data_ptr; }

            inline buffer_binding_args *bindings() const noexcept { return binding_ptr; }

        private:
            object_ref(void *data_ptr, buffer_binding_args *binding_ptr, std::size_t push_data_size,
                       std::size_t offsets_size) noexcept:
                    data_ptr(data_ptr), binding_ptr(binding_ptr), push_data_size(push_data_size),
                    offsets_size(offsets_size) {}

            void *data_ptr = nullptr;
            buffer_binding_args *binding_ptr = nullptr;
            std::size_t push_data_size = 0;
            std::size_t offsets_size = 0;

            //ideally youd compare all values, but this class will never be used outside of pipelines
            //and for their use case, comparing only data_ptr should be fine
            //binding_ptr cannot be compared as there can be 0 bindings, meaning both are always nullptr
            friend bool operator==(const object_ref &a, const object_ref &b) { return a.data_ptr == b.data_ptr; };

            friend bool operator!=(const object_ref &a, const object_ref &b) { return a.data_ptr != b.data_ptr; };

            friend class object_vector;

            friend class object_vector::iterator;
        };

        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = object_ref;
            using pointer = object_ref *;
            using reference = object_ref &;

            inline reference operator*() { return current; }

            inline pointer operator->() { return &current; }

            inline iterator &operator++() {
                current.data_ptr = static_cast<std::byte *>(current.data_ptr) +
                                   push_data_size + offsets_size + element_base_size;
                current.binding_ptr += binding_increment;
                return *this;
            }

            inline iterator operator++(int) {
                iterator tmp = *this;
                current.data_ptr = static_cast<std::byte *>(current.data_ptr) +
                                   push_data_size + offsets_size + element_base_size;
                current.binding_ptr += binding_increment;
                return tmp;
            }

            friend inline bool operator==(const iterator &a, const iterator &b) { return a.current == b.current; };

            friend inline bool operator!=(const iterator &a, const iterator &b) { return a.current != b.current; };

        private:
            iterator() = delete;

            iterator(void *objects, buffer_binding_args *bindings, std::size_t object_increment,
                     u32 binding_increment, std::size_t push_data_size, std::size_t offsets_size) noexcept:
                    current(objects, bindings, push_data_size, offsets_size), object_increment(object_increment),
                    binding_increment(binding_increment), push_data_size(push_data_size),
                    offsets_size(offsets_size) {}

            object_ref current;

            std::size_t object_increment = 0;
            u32 binding_increment = 0;

            std::size_t push_data_size = 0;
            std::size_t offsets_size = 0;

            friend class object_vector;
        };

        object_ref add();

        inline iterator begin() {
            return iterator(object_data, object_descriptor_data, element_stride, n_descriptors, push_data_size,
                            n_dynamic_descriptors * sizeof(offset_t));
        }

        inline iterator end() {
            void *objects = static_cast<std::byte *>(object_data) + count * element_stride;
            buffer_binding_args *bindings = object_descriptor_data + count * n_descriptors;
            return iterator(objects, bindings, element_stride, n_descriptors, push_data_size,
                            n_dynamic_descriptors * sizeof(offset_t));
        }

        inline object_ref operator[](std::size_t idx) {
            INTERNAL_ASSERT(idx < count, "Index out of bounds");
            void *object_p = static_cast<std::byte *>(object_data) + idx * element_stride;
            buffer_binding_args *binding_p = object_descriptor_data + idx * n_descriptors;
            return {object_p, binding_p, push_data_size, n_dynamic_descriptors * sizeof(offset_t)};
        }

    private:
        std::size_t count = 0;
        std::size_t capacity = 0;

        std::size_t push_data_size = 0;
        u32 n_descriptors = 0;
        u32 n_dynamic_descriptors = 0;

        std::size_t element_stride = 0;
        std::size_t descriptor_stride = 0;

        void *object_data = nullptr;

        // bindings are only used when updating descriptors, so they're stored in a separate buffer
        buffer_binding_args *object_descriptor_data = nullptr;

        // smp is a temporary object used for swapping, it is part of the class to avoid allocating memory with every swap
        void *smp = nullptr;

        static const std::size_t element_base_size = sizeof(base_object);
    };
}
