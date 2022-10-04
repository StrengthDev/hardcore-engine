#pragma once

#include "core.hpp"
#include "layer.hpp"
#include "time.hpp"

namespace ENGINE_NAMESPACE
{
	ENGINE_API Layer* push_layer(Layer* layer);
	ENGINE_API void pop_layer();
	ENGINE_API Layer* push_overlay(Layer* layer);
	ENGINE_API void pop_overlay();
	ENGINE_API void clear_layers();
	ENGINE_API void shutdown();
	ENGINE_API time_t delta_time();
	ENGINE_API duration elapsed_time();
}
