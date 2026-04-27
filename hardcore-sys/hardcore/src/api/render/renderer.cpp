
#include <pch.hpp>

#include <render/renderer.h>

#include <render/renderer.hpp>
#include <render/vulkan.hpp>

#include <util/number.hpp>

struct VersionBitfield {
    u32 patch : 12;
    u32 minor : 10;
    u32 major : 7;
    u32 variant : 3;
};

static constexpr HCVersion bitfield_to_version(u32 version_bitfield) {
    auto [patch, minor, major, variant] = std::bit_cast<VersionBitfield>(version_bitfield);
    return {.major = major, .minor = minor, .patch = patch,};
}

const HCVersion HC_VULKAN_API_VERSION = bitfield_to_version(hc::render::VULKAN_API_VERSION);
const HCVersion HC_VULKAN_HEADERS_VERSION = bitfield_to_version(VK_HEADER_VERSION_COMPLETE);
const u32 HC_VOLK_HEADER_VERSION = VOLK_HEADER_VERSION;

HCResult hc_render_tick() {
    auto result = hc::render::tick();
    if (!result) {
        return result.error();
    }

    return {.success = true};
}

HCResult hc_render_finish() {
    auto result = hc::render::finish();
    if (!result) {
        return result.error();
    }

    return {.success = true};
}
