#pragma once

#include "Core.hpp"
#include "Layer.hpp"
#include "Window.hpp"
#include "Spiral/Test.hpp"

namespace Spiral
{
	class SPIRAL_API Client
	{
	public:
		Client();

		virtual ~Client();

		void pushEvent(Event e);

		void pushLayer(Layer* layer);

		inline void popLayer();

		void pushOverlay(Layer* layer);

		inline void popOverlay();

		inline void clearLayers();

		void run();

		inline Window& getWindow() { return *window; };

		inline static Client& get() { return *instance; };

	private:
		Window* window;

		Layer** layerStack;
		uint16_t layerStackCapacity;
		uint16_t nLayers;
		uint16_t nOverlays;
		uint16_t overlayOffset;

		uint16_t popLayerBuffer;
		uint16_t popOverlayBuffer;
		Layer** pushLayerBuffer;
		uint16_t pushLayerCapacity;
		uint16_t pushLayerSize;
		Layer** pushOverlayBuffer;
		uint16_t pushOverlayCapacity;
		uint16_t pushOverlaySize;

		bool running;

		Event* eventBuffer;
		uint16_t eventBufferSize;
		uint16_t eventStart; //Inclusive
		uint16_t eventEnd; //Non-inclusive


		static Client* instance; //There will be only one Client class instance during runtime
	};

	//Should be defined in client
	Client* start();
}