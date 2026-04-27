
#include <pch.hpp>

#include "buffer.hpp"

#include <util/number.hpp>

namespace hc::render::buffer {
    Buffer::Buffer(device::memory::BufferRef const& ref)
        : ref(ref) {}

    device::memory::BufferRef const& Buffer::memory_ref() const noexcept {
        return this->ref;
    }

    Sz Buffer::content_offset() const noexcept {
        return this->ref.offset + this->ref.padding;
    }

    DynamicBuffer::DynamicBuffer(device::memory::DynamicBufferRef const& ref)
        : ref(ref) {}

    device::memory::DynamicBufferRef const& DynamicBuffer::memory_ref() const noexcept {
        return this->ref;
    }

    Sz DynamicBuffer::content_offset() const noexcept {
        return ref.offset + ref.padding;
    }
}
