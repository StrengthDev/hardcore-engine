#pragma once

#include "core.hpp"
#include "layer.hpp"
#include "time.hpp"
#include "static_client.hpp"
#include <spiral/render/renderer.hpp>

namespace Spiral
{
	class SPIRAL_API client
	{
	public:
		client() = delete;
		client(const client& other) = delete;
		client(const char* name, const unsigned int major_version, const unsigned int minor_version, const unsigned int patch_version);
		virtual ~client();

		client& operator=(const client& other) = delete;

		void push_event(Event&& e);
		void push_window_size(int width, int height);

		Layer* push_layer(Layer* layer);
		void pop_layer();
		Layer* push_overlay(Layer* layer);
		void pop_overlay();
		void clear_layers();

		void run();
		void shutdown();

		inline Renderer& getRenderer() const { return *renderer; };

		typedef uint16_t index_t;

		static const index_t event_buffer_capacity = BIT(7);
		static const index_t initial_stack_capacity = 5;

	private:
		void handle_event();


		Renderer* renderer;
		program_id client_id;

		Layer** layer_stack;
		index_t layer_stack_capacity;
		index_t n_layers;
		index_t n_overlays;

		index_t pop_layer_buffer;
		index_t pop_overlay_buffer;
		Layer** push_layer_buffer;
		index_t push_layer_capacity;
		index_t push_layer_size;
		Layer** push_overlay_buffer;
		index_t push_overlay_capacity;
		index_t push_overlay_size;

		bool running;

		time_t delta_time = 0;
		duration elapsed_time;

		Event event_buffer[event_buffer_capacity] = {};
		index_t event_buffer_size;
		index_t event_start; //Inclusive
		index_t event_end; //Exclusive

		int window_width;
		int window_height;
		bool window_size_changed;


		static client* instance; //There will be only one Client class instance during runtime
		static program_id engine_id;

		friend Layer* Spiral::push_layer(Layer* layer);
		friend void Spiral::pop_layer();
		friend Layer* Spiral::push_overlay(Layer* layer);
		friend void Spiral::pop_overlay();
		friend void Spiral::clear_layers();
		friend void Spiral::shutdown();
		friend time_t Spiral::delta_time();
		friend duration Spiral::elapsed_time();
	};

	//Should be defined in client
	client* start();
}
