#pragma once

#include "core.hpp"
#include "layer.hpp"
#include "window.hpp"
#include <spiral/render/renderer.hpp>


static const char* SPIRAL_NAME = "Spiral Engine";
static const unsigned int SPIRAL_MAJOR_VERSION = 1;
static const unsigned int SPIRAL_MINOR_VERSION = 0;
static const unsigned int SPIRAL_PATCH_VERSION = 0;

using index_t = uint16_t;

static const index_t EVENT_BUFFER_CAPACITY = 128;
static const index_t INITIAL_STACK_CAPACITY = 5;

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
		index_t layerStackCapacity;
		index_t nLayers;
		index_t nOverlays;
		index_t overlayOffset;

		index_t popLayerBuffer;
		index_t popOverlayBuffer;
		Layer** pushLayerBuffer;
		index_t pushLayerCapacity;
		index_t pushLayerSize;
		Layer** pushOverlayBuffer;
		index_t pushOverlayCapacity;
		index_t pushOverlaySize;

		bool running;

		Event eventBuffer[EVENT_BUFFER_CAPACITY];
		index_t eventBufferSize;
		index_t eventStart; //Inclusive
		index_t eventEnd; //Non-inclusive

		int windowWidth;
		int windowHeight;
		bool windowSizeChanged;


		static Client* instance; //There will be only one Client class instance during runtime
		static ECProperties engineProperties;
	};

	//Should be defined in client
	Client* start();
}