#include <pch.hpp>

#include "call_instances.hpp"

namespace ENGINE_NAMESPACE {


    object_vector::object_vector(std::size_t push_data_size, u32 n_descriptors,
                                 u32 n_dynamic_descriptors) :
            count(0), capacity(4), push_data_size(push_data_size), n_descriptors(n_descriptors),
            n_dynamic_descriptors(n_dynamic_descriptors) {
        element_stride = element_base_size + push_data_size + n_dynamic_descriptors * sizeof(offset_t);
        descriptor_stride = n_descriptors * sizeof(buffer_binding_args);
        object_data = ex_malloc(capacity * element_stride);
        if (n_descriptors)
            object_descriptor_data = t_malloc<buffer_binding_args>(capacity * n_descriptors);
        smp = ex_malloc(std::max(element_stride, descriptor_stride));
    }

    object_vector::~object_vector() {
        std::free(object_data);
        std::free(object_descriptor_data);
        std::free(smp);
    }

    object_vector::object_ref object_vector::add() {
        std::size_t idx = count++;

        if (count >= capacity) {
            capacity *= 2;
            object_data = ex_realloc(object_data, capacity * element_stride);
            object_descriptor_data = t_realloc<buffer_binding_args>(object_descriptor_data, capacity * n_descriptors);
        }

        void *object_p = static_cast<std::byte *>(object_data) + idx * element_stride;
        buffer_binding_args *binding_p = object_descriptor_data + idx * n_descriptors;
        std::memset(binding_p, 0, descriptor_stride);
        return object_ref(object_p, binding_p, push_data_size, n_dynamic_descriptors * sizeof(offset_t));
    }

    void object_vector::remove(std::size_t idx) {
        //TODO
    }

    void object_vector::swap(std::size_t a, std::size_t b) {
        void *aop = static_cast<std::byte *>(object_data) + element_stride * a;
        void *bop = static_cast<std::byte *>(object_data) + element_stride * b;
        void *adp = object_descriptor_data + n_descriptors * a;
        void *bdp = object_descriptor_data + n_descriptors * b;

        std::memcpy(smp, aop, element_stride);
        std::memcpy(aop, bop, element_stride);
        std::memcpy(bop, smp, element_stride);
        std::memcpy(smp, adp, descriptor_stride);
        std::memcpy(adp, bdp, descriptor_stride);
        std::memcpy(bdp, smp, descriptor_stride);
    }
}
