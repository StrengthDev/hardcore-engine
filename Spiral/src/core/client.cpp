#include <pch.hpp>

#include <spiral/core/client.hpp>
#include <spiral/core/window.hpp>
#include <spiral/core/window_internal.hpp>
#include <spiral/parallel/thread_manager.hpp>

namespace Spiral
{
	program_id client::engine_id = {
		ENGINE_NAME,
		MAJOR_VERSION,
		MINOR_VERSION,
		PATCH_VERSION
	};

	client* client::instance = nullptr;

	client::client(const char* name, const unsigned int major_version, const unsigned int minor_version, const unsigned int patch_version) //TODO: might want to consider giving an absolute size to the layerstack and move it to the stack memory space instead of heap
	{
		instance = this;
		layer_stack = t_malloc<Layer*>(initial_stack_capacity);
		layer_stack_capacity = initial_stack_capacity;
		n_layers = 0;
		n_overlays = 0;

		pop_layer_buffer = 0;
		pop_overlay_buffer = 0;
		push_layer_buffer = t_malloc<Layer*>(initial_stack_capacity);
		push_layer_capacity = initial_stack_capacity;
		push_layer_size = 0;
		push_overlay_buffer = t_malloc<Layer*>(initial_stack_capacity);
		push_overlay_capacity = initial_stack_capacity;
		push_overlay_size = 0;

		running = true;

		window::init(std::bind(&client::push_event, this, std::placeholders::_1), std::bind(&client::push_window_size, this, std::placeholders::_1, std::placeholders::_2));
		window::get_dimensions(&window_width, &window_height);
		window_size_changed = false;

		client_id.name = name;
		client_id.major = major_version;
		client_id.minor = minor_version;
		client_id.patch = patch_version;

		event_buffer_size = 0;
		event_start = 0;
		event_end = 0;
		
		renderer = Renderer::init(engine_id, client_id);
		parallel::launch_threads();
	}

	client::~client()
	{
		delete renderer;
		const index_t total = n_layers + n_overlays;
		index_t i;
		for (i = 1; i < total + 1; i++)
		{
			delete layer_stack[total - i];
		}
		parallel::terminate_threads();
		free(layer_stack);
		free(push_layer_buffer);
		free(push_overlay_buffer);
	}

	static std::mutex eventMutex;

	void client::push_event(const Event& e)
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		if (event_buffer_size < event_buffer_capacity)
		{
			event_buffer[event_end] = e;
			//eventEnd = eventEnd + 1 == EVENT_BUFFER_CAPACITY ? 0 : eventEnd + 1; //because the capacity is fairly big, comparing is better than using the % operator (if the capacity was not a power of 2)
			event_end = (event_end + 1) % event_buffer_capacity; //modulus on a power of 2 is really fast
			event_buffer_size++;
		}
		else
		{
			LOG_INTERNAL_WARN("Event buffer overflow!");
		}
	}

	void client::push_window_size(int width, int height)
	{
		window_width = width;
		window_height = height;
		window_size_changed = true;
	}

	Layer* client::push_layer(Layer *layer)
	{
		if (push_layer_size + 1 > push_layer_capacity)
		{
			push_layer_capacity += initial_stack_capacity;
			Layer** t = t_malloc<Layer*>(push_layer_capacity);
			memcpy(t, push_layer_buffer, sizeof(Layer*) * push_layer_size);
			free(push_layer_buffer);
			push_layer_buffer = t;
		}
		push_layer_buffer[push_layer_size] = layer;
		push_layer_size++;
		return layer;
	}

	void client::pop_layer()
	{
		pop_layer_buffer++;
	}

	Layer* client::push_overlay(Layer *layer)
	{
		if (push_overlay_size + 1 > push_overlay_capacity)
		{
			push_overlay_capacity += initial_stack_capacity;
			Layer** t = t_malloc<Layer*>(push_overlay_capacity);
			memcpy(t, push_overlay_buffer, sizeof(Layer*) * push_overlay_size);
			free(push_overlay_buffer);
			push_overlay_buffer = t;
		}
		push_overlay_buffer[push_overlay_size] = layer;
		push_overlay_size++;
		return layer;
	}

	void client::pop_overlay()
	{
		pop_overlay_buffer++;
	}

	void client::clear_layers()
	{
		pop_layer_buffer = n_layers;
	}

	void client::run()
	{
		index_t i, n;
		Layer** t = nullptr;
		while (running)
		{
			for (i = 0; i < n_layers + n_overlays; i++)
			{
				layer_stack[i]->tick();
			}

			window::tick();
			renderer->m_presentFrame();

			if (window_size_changed)
			{
				push_event(Event::windowResize(window_width, window_height));
				window_size_changed = false;
			}
			
			while (event_buffer_size > 0) //TODO: replace goto with function
			{
				for (i = n_layers + n_overlays - 1; i != std::numeric_limits<index_t>::max(); i--)
				{
					if (layer_stack[i]->handleEvent(event_buffer[event_start])) //TODO: place the switch for event types here, and implement an event function for each type in the layer class, to avoid multiple switches per tick
					{
						goto endevent;
					}
				}
				
				if (event_buffer[event_start].type == EventType::WindowClose)
				{
					running = false;
				}

				endevent:
				event_start = (event_start + 1) % event_buffer_capacity;
				event_buffer_size--;
			}

			if (pop_overlay_buffer)
			{
				n = n_layers + n_overlays;
				for (i = n - pop_overlay_buffer; i < n; i++) 
				{
					delete layer_stack[i];
				}
				n_overlays -= pop_overlay_buffer;
				pop_overlay_buffer = 0;
			}

			if (pop_layer_buffer)
			{
				for (i = n_layers - pop_overlay_buffer; i < n_layers; i++)
				{
					delete layer_stack[i];
				}
				n_layers -= pop_layer_buffer;
				memmove(&layer_stack[n_layers], &layer_stack[n_layers + pop_layer_buffer], sizeof(Layer*) * n_overlays);
				pop_layer_buffer = 0;
			}

			if (push_layer_size || push_overlay_size)
			{
				n = push_layer_size + push_overlay_size + n_layers + n_overlays;
				if (n > layer_stack_capacity)
				{
					i = (n / initial_stack_capacity + 1) * initial_stack_capacity;
					t = t_malloc<Layer*>(i);
					layer_stack_capacity = i;
					memcpy(t, layer_stack, sizeof(Layer*) * ((std::size_t)n_layers + n_overlays)); //There was an arithmetic overflow warning and it was annoying me
					free(layer_stack);
					layer_stack = t;
				}

				if (push_layer_size)
				{
					memmove(&layer_stack[n_layers + push_layer_size], &layer_stack[n_layers], sizeof(Layer*) * n_overlays);
					memcpy(&layer_stack[n_layers], push_layer_buffer, sizeof(Layer*) * push_layer_size);
					n_layers += push_layer_size;
					push_layer_size = 0;
				}

				if (push_overlay_size)
				{
					n = n_layers + n_overlays;
					memcpy(&layer_stack[n_layers + n_overlays], push_overlay_buffer, sizeof(Layer*) * push_overlay_size);
					n_overlays += push_overlay_size;
					push_overlay_size = 0;
				}
			}
		}
	}

	void client::shutdown()
	{
		running = false;
	}
}
