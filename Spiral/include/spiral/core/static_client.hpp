#pragma once

#include "core.hpp"
#include "layer.hpp"
#include "time.hpp"

namespace Spiral
{
	SPIRAL_API Layer* push_layer(Layer* layer);
	SPIRAL_API void pop_layer();
	SPIRAL_API Layer* push_overlay(Layer* layer);
	SPIRAL_API void pop_overlay();
	SPIRAL_API void clear_layers();
	SPIRAL_API void shutdown();
	SPIRAL_API time_t delta_time();
	SPIRAL_API duration elapsed_time();
}
