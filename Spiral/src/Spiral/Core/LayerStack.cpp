#include "pch.hpp"

#include "LayerStack.hpp"

namespace Spiral
{
	LayerStack::~LayerStack()
	{
		size_t i;
		size_t n_layers = basestack.size();
		for (i = 0; i < n_layers; i++)
		{
			delete basestack[i];
		}
		size_t n_overlays = overlaystack.size();
		for (i = 0; i < n_overlays; i++)
		{
			delete overlaystack[i];
		}
	}

	void LayerStack::pushLayer(Spiral::Layer* layer)
	{
		basestack.emplace_back(layer);
	}

	void LayerStack::pushOverlay(Spiral::Layer* layer)
	{
		overlaystack.emplace_back(layer);
	}

	void LayerStack::popLayer()
	{
		delete basestack.back();
		basestack.pop_back();
	}

	void LayerStack::popOverlay()
	{
		delete overlaystack.back();
		overlaystack.pop_back();
	}

	void LayerStack::clear()
	{
		size_t i;
		size_t n_layers = basestack.size();
		for (i = 0; i < n_layers; i++)
		{
			delete basestack[i];
		}
		basestack.clear();
	}
}