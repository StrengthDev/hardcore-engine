/**
 * @file
 * @brief This file is dedicated to functions called inside render object implementations.
 */

#pragma once

#include <core/util.hpp>

namespace hc::render {
    u8 max_frames_in_flight();

    u8 current_frame_mod();
}

