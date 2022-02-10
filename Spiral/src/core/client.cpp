#include <pch.hpp>

#include <spiral/core/client.hpp>
#include <spiral/parallel/thread_manager.hpp>

namespace Spiral
{
	program_id Client::engine_id = {
		ENGINE_NAME,
		MAJOR_VERSION,
		MINOR_VERSION,
		PATCH_VERSION
	};

	Client* Client::instance = nullptr;

	Client::Client(const char* name, const unsigned int major_version, const unsigned int minor_version, const unsigned int patch_version) //TODO: might want to consider giving an absolute size to the layerstack and move it to the stack memory space instead of heap
	{
		instance = this;
		layerStack = (Layer**)malloc(sizeof(Layer*) * INITIAL_STACK_CAPACITY);
		layerStackCapacity = INITIAL_STACK_CAPACITY;
		nLayers = 0;
		nOverlays = 0;
		overlayOffset = 0;

		popLayerBuffer = 0;
		popOverlayBuffer = 0;
		pushLayerBuffer = (Layer**)malloc(sizeof(Layer*) * INITIAL_STACK_CAPACITY);
		pushLayerCapacity = INITIAL_STACK_CAPACITY;
		pushLayerSize = 0;
		pushOverlayBuffer = (Layer**)malloc(sizeof(Layer*) * INITIAL_STACK_CAPACITY);
		pushOverlayCapacity = INITIAL_STACK_CAPACITY;
		pushOverlaySize = 0;

		running = true;

		window = Window::init();
		window->setEventCallback(std::bind(&Client::push_event, this, std::placeholders::_1)); //because pushEvent is a member function, a forwarder function must be used 
		window->setSizeCallback(std::bind(&Client::push_window_size, this, std::placeholders::_1, std::placeholders::_2));

		window->getDimensions(&windowWidth, &windowHeight);
		windowSizeChanged = false;

		client_id.name = name;
		client_id.major = major_version;
		client_id.minor = minor_version;
		client_id.patch = patch_version;

		eventBufferSize = 0;
		eventStart = 0;
		eventEnd = 0;
		
		renderer = Renderer::init(engine_id, client_id);
		parallel::launch_threads();
	}

	Client::~Client()
	{
		delete renderer;
		delete window;
		const index_t total = nLayers + nOverlays;
		index_t i;
		for (i = 1; i < total + 1; i++)
		{
			delete layerStack[total - i];
		}
		parallel::terminate_threads();
		free(layerStack);
		free(pushLayerBuffer);
		free(pushOverlayBuffer);
	}

	static std::mutex eventMutex;

	void Client::push_event(const Event& e)
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		if (eventBufferSize < EVENT_BUFFER_CAPACITY)
		{
			eventBuffer[eventEnd] = e;
			//eventEnd = eventEnd + 1 == EVENT_BUFFER_CAPACITY ? 0 : eventEnd + 1; //because the capacity is fairly big, comparing is better than using the % operator (if the capacity was not a power of 2)
			eventEnd = (eventEnd + 1) % EVENT_BUFFER_CAPACITY; //modulus on a power of 2 is really fast
			eventBufferSize++;
		}
		else
		{
			LOG_INTERNAL_WARN("Event buffer overflow!");
		}
	}

	void Client::push_window_size(int width, int height)
	{
		windowWidth = width;
		windowHeight = height;
		windowSizeChanged = true;
	}

	Layer* Client::push_layer(Layer *layer)
	{
		if (pushLayerSize + 1 > pushLayerCapacity)
		{
			pushLayerCapacity += INITIAL_STACK_CAPACITY;
			Layer** t = (Layer**)malloc(sizeof(Layer*) * pushLayerCapacity);
			/*for (index_t i = 0; i < pushLayerSize; i++)
			{
				t[i] = pushLayerBuffer[i];
			}*/
			memcpy(t, pushLayerBuffer, sizeof(Layer*) * pushLayerSize);
			free(pushLayerBuffer);
			pushLayerBuffer = t;
		}
		pushLayerBuffer[pushLayerSize] = layer;
		pushLayerSize++;
		return layer;
	}

	void Client::pop_layer()
	{
		popLayerBuffer++;
	}

	Layer* Client::push_overlay(Layer *layer)
	{
		if (pushOverlaySize + 1 > pushOverlayCapacity)
		{
			pushOverlayCapacity += INITIAL_STACK_CAPACITY;
			Layer** t = (Layer**)malloc(sizeof(Layer*) * pushOverlayCapacity);
			/*for (index_t i = 0; i < pushOverlaySize; i++)
			{
				t[i] = pushOverlayBuffer[i];
			}*/
			memcpy(t, pushOverlayBuffer, sizeof(Layer*) * pushOverlaySize);
			free(pushOverlayBuffer);
			pushOverlayBuffer = t;
		}
		pushOverlayBuffer[pushOverlaySize] = layer;
		pushOverlaySize++;
		return layer;
	}

	void Client::pop_overlay()
	{
		popOverlayBuffer++;
	}

	void Client::clear_layers()
	{
		popLayerBuffer = nLayers;
	}

	void Client::run()
	{
		index_t i, n;
		Layer** t = nullptr;
		while (running)
		{
			for (i = 0; i < nLayers + nOverlays; i++)
			{
				layerStack[i]->tick();
			}

			window->tick();
			renderer->m_presentFrame();

			if (windowSizeChanged)
			{
				push_event(Event::windowResize(windowWidth, windowHeight));
				windowSizeChanged = false;
			}
			
			while (eventBufferSize > 0) //TODO: replace goto with function
			{
				for (i = nLayers + nOverlays - 1; i != std::numeric_limits<index_t>::max(); i--)
				{
					if (layerStack[i]->handleEvent(eventBuffer[eventStart])) //TODO: place the switch for event types here, and implement an event function for each type in the layer class, to avoid multiple switches per tick
					{
						goto endevent;
					}
				}
				
				if (eventBuffer[eventStart].type == EventType::WindowClose)
				{
					running = false;
				}

				endevent:
				eventStart = (eventStart + 1) % EVENT_BUFFER_CAPACITY;
				eventBufferSize--;
			}

			if (popOverlayBuffer)
			{
				n = nLayers + nOverlays;
				for (i = n - popOverlayBuffer; i < n; i++) 
				{
					delete layerStack[i];
				}
				nOverlays -= popOverlayBuffer;
				popOverlayBuffer = 0;
			}

			if (popLayerBuffer)
			{
				for (i = nLayers - popOverlayBuffer; i < nLayers; i++)
				{
					delete layerStack[i];
				}
				nLayers -= popLayerBuffer;
				/*for (i = nLayers; i < nLayers + nOverlays; i++)
				{
					layerStack[i] = layerStack[i + popLayerBuffer];
				}*/
				memmove(&layerStack[nLayers], &layerStack[nLayers + popLayerBuffer], sizeof(Layer*) * nOverlays);
				popLayerBuffer = 0;
			}

			if (pushLayerSize || pushOverlaySize)
			{
				n = pushLayerSize + pushOverlaySize + nLayers + nOverlays;
				if (n > layerStackCapacity)
				{
					i = (n / INITIAL_STACK_CAPACITY + 1) * INITIAL_STACK_CAPACITY;
					t = (Layer**)malloc(sizeof(Layer*) * i);
					layerStackCapacity = i;
					/*for (i = 0; i < nLayers + nOverlays; i++)
					{
						t[i] = layerStack[i];
					}*/
					memcpy(t, layerStack, sizeof(Layer*) * (nLayers + nOverlays));
					free(layerStack);
					layerStack = t;
				}

				if (pushLayerSize)
				{
					/*for (i = nLayers + nOverlays + pushLayerSize - 1; i > nLayers + pushLayerSize - 1; i--)
					{
						layerStack[i] = layerStack[i - pushLayerSize];
					}*/
					memmove(&layerStack[nLayers + pushLayerSize], &layerStack[nLayers], sizeof(Layer*) * nOverlays);
					/*for (i = nLayers; i < nLayers + pushLayerSize; i++)
					{
						layerStack[i] = pushLayerBuffer[i - nLayers];
					}*/
					memcpy(&layerStack[nLayers], pushLayerBuffer, sizeof(Layer*) * pushLayerSize);
					nLayers += pushLayerSize;
					pushLayerSize = 0;
				}

				if (pushOverlaySize)
				{
					n = nLayers + nOverlays;
					/*for (i = n; i < n + pushOverlaySize; i++)
					{
						layerStack[i] = pushOverlayBuffer[i - n];
					}*/
					memcpy(&layerStack[nLayers + nOverlays], pushOverlayBuffer, sizeof(Layer*) * pushOverlaySize);
					nOverlays += pushOverlaySize;
					pushOverlaySize = 0;
				}
			}
		}
	}

	void Client::shutdown()
	{
		running = false;
	}
}