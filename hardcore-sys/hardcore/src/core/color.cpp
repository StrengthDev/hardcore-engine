
#include <pch.hpp>

#include "color.hpp"
#include "log.hpp"

namespace hc {
    HCColor clamp_color(HCColor color) {
        HCColor const ret = {
            .r = std::clamp(color.r, 0.0f, 1.0f),
            .g = std::clamp(color.g, 0.0f, 1.0f),
            .b = std::clamp(color.b, 0.0f, 1.0f),
            .a = std::clamp(color.a, 0.0f, 1.0f),
        };

        if (color.r != ret.r || color.g != ret.g || color.b != ret.b || color.a != ret.a) {
            HC_WARN("Clamped color component values");
        }

        return ret;
    }
}
