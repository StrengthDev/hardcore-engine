#pragma once

#include <util/number.hpp>

namespace hc::render::buffer {
    /**
     * @brief The inner parameters of a resource buffer.
     */
    struct Params {
        u64 id; //!< The ID of this buffer within the device.
        Sz size; //!< The amount of usable memory occupied by this buffer, in bytes.
    };

    /**
     * @brief The inner parameters of a dynamic resource buffer.
     */
    struct DynamicParams {
        u64 id; //!< The ID of this buffer within the device.
        Sz size; //!< The amount of usable memory occupied by this buffer, in bytes.
        void **data; //!< A pointer to the underlying buffer backing this buffer's data, to which the host has direct access.
        Sz data_offset; //!< The offset to this buffer's data within the underlying buffer which backs this one.
    };
}
