#include <pch.hpp>

#include <core/log.hpp>
#include <util/number.hpp>
#include <render/buffer.h>

#include "../renderer.hpp"
#include "../vars.hpp"
#include "descriptor.hpp"

const HCBuffer INVALID_BUFFER = {
        .device = std::numeric_limits<DeviceID>::max(),
        .id = std::numeric_limits<GraphItemID>::max(),
        .status = HCBufferStatus::Destroyed,
};

const HCDynamicBuffer INVALID_DYNAMIC_BUFFER = {
        .base = INVALID_BUFFER,
        .data = nullptr,
        .data_offset = std::numeric_limits<u64>::max(),
};

HCBuffer hc_new_buffer(HCBufferKind kind, HCDescriptor *descriptor, u64 count, bool writable, DeviceID device) {
    if (kind == HCBufferKind::Index) {
        HC_ERROR("Invalid buffer kind (call `hc_new_index_buffer` instead)");
        return INVALID_BUFFER;
    }
    if (!descriptor) {
        HC_ERROR("Descriptor cannot be null");
        return INVALID_BUFFER;
    }
    if (!descriptor->fields || !descriptor->field_count) {
        HC_ERROR("Invalid descriptor");
        return INVALID_BUFFER;
    }
    if (!count) {
        HC_ERROR("Invalid element count");
        return INVALID_BUFFER;
    }

    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_ERROR("Tried to allocate buffer when global context is uninitialized");
        return INVALID_BUFFER;
    }
    if (devices.size() - 1 < device) {
        HC_ERROR("Device does not exist");
        return INVALID_BUFFER;
    }
    auto &device_handle = devices[device];

    auto res = device_handle.new_buffer(kind, hc::render::resource::Descriptor(*descriptor), count, writable);
    if (res) {
        auto buffer = std::move(res).ok();
        buffer.device = device;
        return buffer;
    } else {
        return INVALID_BUFFER;
    }
}

HCBuffer hc_new_index_buffer(HCPrimitive index_type, u64 count, bool writable, DeviceID device) {
    if (index_type != HCPrimitive::U8 && index_type != HCPrimitive::U16 && index_type != HCPrimitive::U32) {
        HC_ERROR("Invalid index type");
        return INVALID_BUFFER;
    }
    if (!count) {
        HC_ERROR("Invalid element count");
        return INVALID_BUFFER;
    }

    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_ERROR("Tried to allocate buffer when global context is uninitialized");
        return INVALID_BUFFER;
    }
    if (devices.size() - 1 < device) {
        HC_ERROR("Device does not exist");
        return INVALID_BUFFER;
    }
    auto &device_handle = devices[device];

    auto res = device_handle.new_index_buffer(index_type, count, writable);
    if (res) {
        auto buffer = std::move(res).ok();
        buffer.device = device;
        return buffer;
    } else {
        return INVALID_BUFFER;
    }
}

void hc_destroy_buffer(HCBuffer *buffer) {
    if (!buffer)
        return;

    if (buffer->device == INVALID_BUFFER.device || buffer->id == INVALID_BUFFER.id ||
        buffer->status != HCBufferStatus::Valid) {
        HC_WARN("Attempted to destroy invalid buffer");
    }

    const auto device_id = buffer->device;
    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_WARN("Tried to destroy buffer when global context is uninitialized");
    }
    if (devices.size() - 1 < device_id) {
        HC_WARN("Buffer isn't owned by a valid device");
    }
    auto &device = devices[device_id];
    auto frame_mod = hc::render::current_frame_mod();

    device.destroy_buffer(*buffer, frame_mod);
    *buffer = INVALID_BUFFER;
}

HCDynamicBuffer hc_new_dynamic_buffer(HCBufferKind kind, HCDescriptor *descriptor, u64 count, bool writable,
                                      DeviceID device) {
    if (kind == HCBufferKind::Index) {
        HC_ERROR("Invalid buffer kind (call `hc_new_dynamic_index_buffer` instead)");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (!descriptor) {
        HC_ERROR("Descriptor cannot be null");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (!descriptor->fields || !descriptor->field_count) {
        HC_ERROR("Invalid descriptor");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (!count) {
        HC_ERROR("Invalid element count");
        return INVALID_DYNAMIC_BUFFER;
    }

    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_ERROR("Tried to allocate buffer when global context is uninitialized");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (devices.size() - 1 < device) {
        HC_ERROR("Device does not exist");
        return INVALID_DYNAMIC_BUFFER;
    }
    auto &device_handle = devices[device];

    auto res = device_handle.new_dynamic_buffer(kind, hc::render::resource::Descriptor(*descriptor), count, writable);
    if (res) {
        auto buffer = std::move(res).ok();
        buffer.base.device = device;
        return buffer;
    } else {
        return INVALID_DYNAMIC_BUFFER;
    }
}

HCDynamicBuffer hc_new_dynamic_index_buffer(HCPrimitive index_type, u64 count, bool writable, DeviceID device) {
    if (index_type != HCPrimitive::U8 && index_type != HCPrimitive::U16 && index_type != HCPrimitive::U32) {
        HC_ERROR("Invalid index type");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (!count) {
        HC_ERROR("Invalid element count");
        return INVALID_DYNAMIC_BUFFER;
    }

    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_ERROR("Tried to allocate buffer when global context is uninitialized");
        return INVALID_DYNAMIC_BUFFER;
    }
    if (devices.size() - 1 < device) {
        HC_ERROR("Device does not exist");
        return INVALID_DYNAMIC_BUFFER;
    }
    auto &device_handle = devices[device];

    auto res = device_handle.new_dynamic_index_buffer(index_type, count, writable);
    if (res) {
        auto buffer = std::move(res).ok();
        buffer.base.device = device;
        return buffer;
    } else {
        return INVALID_DYNAMIC_BUFFER;
    }
}

void hc_destroy_dynamic_buffer(HCDynamicBuffer *buffer) {
    if (!buffer)
        return;

    if (buffer->base.device == INVALID_BUFFER.device || buffer->base.id == INVALID_BUFFER.id ||
        buffer->base.status != HCBufferStatus::Valid) {
        HC_WARN("Attempted to destroy invalid buffer");
    }

    const auto device_id = buffer->base.device;
    auto &devices = hc::render::device_list();
    if (devices.empty()) {
        HC_WARN("Tried to destroy buffer when global context is uninitialized");
    }
    if (devices.size() - 1 < device_id) {
        HC_WARN("Buffer isn't owned by a valid device");
    }
    auto &device = devices[device_id];
    auto frame_mod = hc::render::current_frame_mod();

    device.destroy_dynamic_buffer(*buffer, frame_mod);
    *buffer = INVALID_DYNAMIC_BUFFER;
}
