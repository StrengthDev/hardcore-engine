#pragma once

#include "core.hpp"
#include "layer.hpp"
#include "window.hpp"
#include <spiral/render/renderer.hpp>

namespace Spiral
{
	class SPIRAL_API Client
	{
	public:
		Client() = delete;
		Client(const Client& other) = delete;
		Client(const char* name, const unsigned int major_version, const unsigned int minor_version, const unsigned int patch_version);
		virtual ~Client();

		Client& operator=(const Client& other) = delete;

		void init();
		virtual void pushInitialLayers() = 0;

		inline void setProperties(const char *name, const unsigned int major_version, const unsigned int minor_version, const unsigned int patch_version);

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

		using index_t = uint16_t;

		static const index_t EVENT_BUFFER_CAPACITY = BIT(7);
		static const index_t INITIAL_STACK_CAPACITY = 5;

	private:
		Window* window;
		Renderer* renderer;
		program_id client_id;

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
		static program_id engine_id;
	};

	//Should be defined in client
	Client* start();
}