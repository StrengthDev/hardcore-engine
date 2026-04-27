
#include <pch.hpp>

#include <render/device.h>

#include <render/renderer.hpp>

#include <util/number.hpp>

u32 hc_device_count() {
    return static_cast<u32>(hc::render::device_list().size());
}

const char* hc_device_name(u32 device) {
    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return nullptr;
    }

    return (*device_result)->name();
}
