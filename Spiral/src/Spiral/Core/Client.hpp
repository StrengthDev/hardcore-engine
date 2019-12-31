#pragma once

#include "Core.hpp"
#include "LayerStack.hpp"
#include "Spiral/Test.hpp"

#define LAYER_TIERS 1

namespace Spiral
{
	class SPIRAL_API Client
	{
	public:
		Client();

		virtual ~Client();

		void run();

		void pushLayer(Layer* layer, uint32_t tier);

		void popLayer(uint32_t tier);

		inline static Client& get() { return *instance; };

	private:
		LayerStack layerstack;

		static Client* instance;
	};

	//Should be defined in client
	Client* start();
}