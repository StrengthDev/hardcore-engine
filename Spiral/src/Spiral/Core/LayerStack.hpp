#pragma once

#include "Core.hpp"
#include "Layer.hpp"

#include <vector>

namespace Spiral
{
	struct LayerStack
	{
		// because the vectors contain pointers instead of actual data, the pointers must be cleared manually
		std::vector<Layer*> basestack;
		std::vector<Layer*> overlaystack;
		
		LayerStack() = default;

		~LayerStack();

		void pushLayer(Spiral::Layer* layer);

		void pushOverlay(Spiral::Layer* layer);

		void popLayer();

		void popOverlay();

		void clear();
	};
}