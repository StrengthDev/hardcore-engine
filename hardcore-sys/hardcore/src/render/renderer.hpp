#pragma once

namespace hc::render {
    enum class InstanceResult : std::uint32_t {
        Success = 0,
        VolkError,
    };

    InstanceResult init();

    InstanceResult term();
}
