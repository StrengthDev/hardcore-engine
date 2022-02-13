#pragma once

#include "layer.hpp"

namespace Spiral
{
	Layer* push_layer(Layer* layer);
	void pop_layer();
	Layer* push_overlay(Layer* layer);
	void pop_overlay();
	void clear_layers();
	void shutdown();
}
