
#pragma once

#include "node.hpp"

#include <expected>
#include <core/error.hpp>

namespace hc::render::device::graph {
    class Commands {
    public:
        Commands() = delete;

        static std::expected<Commands, Error> create();
    };
}
