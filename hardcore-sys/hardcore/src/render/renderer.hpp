#pragma once

#include "device/device.hpp"

#include <core/core.h>
#include <core/error.hpp>
#include <util/number.hpp>
#include <render/renderer.h>

namespace hc::render {
    [[nodiscard]] std::expected<void, Error> init(const HCApplicationDescriptor& app, const HCRenderParams& params);

    void term();

    [[nodiscard]] VkInstance vk_instance();

    [[nodiscard]] std::vector<device::Device>& device_list() noexcept;

    [[nodiscard]] std::expected<device::Device*, Error> device_at(u32 id) noexcept;
}
