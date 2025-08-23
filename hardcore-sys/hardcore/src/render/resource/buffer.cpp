#include <pch.hpp>

#include "descriptor.hpp"

#include "../renderer.hpp"
#include "../vars.hpp"

#include <render/buffer.h>

#include <core/log.hpp>
#include <util/number.hpp>

HCResult hc_new_buffer(
    HCBuffer* buffer,
    u32 device,
    HCBufferKind kind,
    const HCDescriptor* descriptor,
    u64 count,
    bool writable
) {
    if (!buffer) {
        HC_ERROR("Null buffer pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (kind == HCBufferKind_Index) {
        HC_ERROR("Invalid buffer kind (call `hc_new_index_buffer` instead)");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!descriptor) {
        HC_ERROR("Descriptor cannot be null");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!descriptor->fields || !descriptor->field_count) {
        HC_ERROR("Invalid descriptor");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!count) {
        HC_ERROR("Invalid element count");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto buffer_result = (*device_result)->new_buffer(
        kind,
        hc::render::Descriptor(*descriptor),
        count,
        writable
    );
    if (!buffer_result) {
        return buffer_result.error();
    }

    *buffer = {
        .id = buffer_result->id,
        .size = buffer_result->size,
        .device = device,
    };

    return {.success = true};
}

HCResult hc_new_index_buffer(HCBuffer* buffer, u32 device, HCPrimitive index_type, u64 count, bool writable) {
    if (!buffer) {
        HC_ERROR("Null buffer pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    // if (index_type != HCPrimitive_U8 && index_type != HCPrimitive_U16 && index_type != HCPrimitive_U32) {
    //     HC_ERROR("Invalid index type");
    //     return {.error = HCError_InvalidParams, .success = false};
    // } TODO

    if (!count) {
        HC_ERROR("Invalid element count");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto buffer_result = (*device_result)->new_index_buffer(index_type, count, writable);
    if (!buffer_result) {
        return buffer_result.error();
    }

    *buffer = {
        .id = buffer_result->id,
        .size = buffer_result->size,
        .device = device,
    };

    return {.success = true};
}

void hc_destroy_buffer(HCBuffer* buffer) {
    if (!buffer) {
        HC_WARN("Null buffer pointer");
        return;
    }

    if (!buffer->size) {
        HC_WARN("Attempted to destroy invalid buffer");
        return;
    }

    const auto device_id = buffer->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    (*device_result)->destroy_buffer(buffer->id);
    *buffer = {};
}

HCResult hc_new_dynamic_buffer(
    HCDynamicBuffer* buffer,
    u32 device,
    HCBufferKind kind,
    const HCDescriptor* descriptor,
    u64 count,
    bool writable
) {
    if (!buffer) {
        HC_ERROR("Null buffer pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (kind == HCBufferKind_Index) {
        HC_ERROR("Invalid buffer kind (call `hc_new_dynamic_index_buffer` instead)");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!descriptor) {
        HC_ERROR("Descriptor cannot be null");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!descriptor->fields || !descriptor->field_count) {
        HC_ERROR("Invalid descriptor");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!count) {
        HC_ERROR("Invalid element count");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto frame_mod = hc::render::current_frame_mod();
    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto buffer_result = (*device_result)->new_dynamic_buffer(
        kind,
        hc::render::Descriptor(*descriptor),
        count,
        writable,
        frame_mod
    );
    if (!buffer_result) {
        return buffer_result.error();
    }

    *buffer = {
        .id = buffer_result->id,
        .size = buffer_result->size,
        .data = buffer_result->data,
        .data_offset = buffer_result->data_offset,
        .device = device,
    };

    return {.success = true};
}

HCResult hc_new_dynamic_index_buffer(
    HCDynamicBuffer* buffer,
    u32 device,
    HCPrimitive index_type,
    u64 count,
    bool writable
) {
    if (!buffer) {
        HC_ERROR("Null buffer pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    // if (index_type != HCPrimitive_U8 && index_type != HCPrimitive_U16 && index_type != HCPrimitive_U32) {
    //     HC_ERROR("Invalid index type");
    //     return {.error = HCError_InvalidParams, .success = false};
    // } TODO

    if (!count) {
        HC_ERROR("Invalid element count");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto frame_mod = hc::render::current_frame_mod();
    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto buffer_result = (*device_result)->new_dynamic_index_buffer(index_type, count, writable, frame_mod);
    if (!buffer_result) {
        return buffer_result.error();
    }

    *buffer = {
        .id = buffer_result->id,
        .size = buffer_result->size,
        .data = buffer_result->data,
        .data_offset = buffer_result->data_offset,
        .device = device,
    };

    return {.success = true};
}

void hc_destroy_dynamic_buffer(HCDynamicBuffer* buffer) {
    if (!buffer) {
        HC_WARN("Null buffer pointer");
        return;
    }

    if (!buffer->size) {
        HC_WARN("Attempted to destroy invalid buffer");
        return;
    }

    const auto device_id = buffer->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    (*device_result)->destroy_buffer(buffer->id);
    *buffer = {};
}
