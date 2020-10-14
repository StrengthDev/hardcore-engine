#pragma once

#include "Core.hpp"
#include "Layer.hpp"
#include "Window.hpp"
#include "Spiral/Render/Renderer.hpp"


#define SPIRAL_NAME				"Spiral Engine"
#define SPIRAL_MAJOR_VERSION	1
#define SPIRAL_MINOR_VERSION	0
#define SPIRAL_PATCH_VERSION	0

#define EVENT_BUFFER_CAPACITY 128

namespace Spiral
{
	class SPIRAL_API Client
	{
	public:
		Client();
		virtual ~Client();
		void init();
		virtual void pushInitialLayers() = 0;

		inline void setProperties(const char *name, unsigned int majorVersion, unsigned int minorVersion, unsigned int patchVersion);

		void pushEvent(Event e);
		void pushWindowSize(int width, int height);

		void pushLayer(Layer *layer);
		inline void popLayer();
		void pushOverlay(Layer *layer);
		inline void popOverlay();
		inline void clearLayers();

		void run();
		void shutdown();

		inline Window& getWindow() const { return *window; };
		inline Renderer& getRenderer() const { return *renderer; };
		inline static Client& get() { return *instance; };

	private:
		Window* window;
		Renderer* renderer;
		ECProperties properties;

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

		Event eventBuffer[EVENT_BUFFER_CAPACITY];
		uint16_t eventBufferSize;
		uint16_t eventStart; //Inclusive
		uint16_t eventEnd; //Non-inclusive

		int windowWidth;
		int windowHeight;
		bool windowSizeChanged;


		static Client* instance; //There will be only one Client class instance during runtime
		static ECProperties engineProperties;
	};

	//Should be defined in client
	Client* start();
}