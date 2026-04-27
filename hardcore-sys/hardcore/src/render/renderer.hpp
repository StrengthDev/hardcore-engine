#pragma once

#include "device/device.hpp"

#include <core/core.h>
#include <core/error.hpp>
#include <util/number.hpp>
#include <render/renderer.h>

namespace hc::render {
    u32 constexpr VULKAN_API_VERSION = VK_API_VERSION_1_3;

    [[nodiscard]] std::expected<void, Error> init(const HCApplicationDescriptor& app, const HCRenderParams& params);

    void term();

    [[nodiscard]] std::expected<void, Error> tick();
    [[nodiscard]] std::expected<void, Error> finish();

    [[nodiscard]] VkInstance vk_instance();

    [[nodiscard]] std::vector<device::Device>& device_list() noexcept;

    [[nodiscard]] std::expected<device::Device*, Error> device_at(u32 id) noexcept;
}
