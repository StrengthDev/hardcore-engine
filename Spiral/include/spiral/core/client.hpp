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

		void push_event(const Event& e);
		void push_window_size(int width, int height);

		Layer* push_layer(Layer* layer);
		inline void pop_layer();
		Layer* push_overlay(Layer* layer);
		inline void pop_overlay();
		inline void clear_layers();

		void run();
		void shutdown();

		inline Window& getWindow() const { return *window; };
		inline Renderer& getRenderer() const { return *renderer; };
		inline static Client& get() { return *instance; };

		typedef uint16_t index_t;

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