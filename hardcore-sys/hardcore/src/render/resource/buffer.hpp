#pragma once

#include "../device/memory/reference.hpp"

#include <util/number.hpp>

namespace hc::render::buffer {
    class Buffer {
    public:
        explicit Buffer(device::memory::BufferRef const& ref);

        [[nodiscard]] device::memory::BufferRef const& memory_ref() const noexcept;

        [[nodiscard]] Sz content_offset() const noexcept;

    private:
        device::memory::BufferRef ref;
    };

    struct BufferData {
        u64 id;
        Sz size;
    };

    struct DynamicBufferData {
        u64 id;
        Sz size;
        void** map_ptr;
        Sz map_offset;
    };

    class DynamicBuffer {
    public:
        explicit DynamicBuffer(device::memory::DynamicBufferRef const& ref);

        [[nodiscard]] device::memory::DynamicBufferRef const& memory_ref() const noexcept;

        [[nodiscard]] Sz content_offset() const noexcept;

    private:
        device::memory::DynamicBufferRef ref;
    };
}
