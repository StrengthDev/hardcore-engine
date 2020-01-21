#include "pch.hpp"

#include "Client.hpp"

#define EVENT_BUFFER_CAPACITY 100
#define INITIAL_STACK_CAPACITY 5

namespace Spiral
{
	Client* Client::instance = nullptr;

	Client::Client()
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
		window->setEventCallback(std::bind(&Client::pushEvent, this, std::placeholders::_1)); //because pushEvent is a member function, a forwarder function must be used 

		eventBuffer = (Event*)malloc(sizeof(Event) * EVENT_BUFFER_CAPACITY);
		eventBufferSize = 0;
		eventStart = 0;
		eventEnd = 0;
	}

	Client::~Client()
	{
		uint16_t total = nLayers + nOverlays;
		uint16_t i;
		for (i = 0; i < total; i++)
		{
			delete layerStack[i];
		}

		free(layerStack);
		free(pushLayerBuffer);
		free(pushOverlayBuffer);
		free(eventBuffer);
	}

	static std::mutex eventMutex;

	void Client::pushEvent(Event e)
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		if (eventBufferSize < EVENT_BUFFER_CAPACITY)
		{
			eventBuffer[eventEnd] = e;
			eventEnd = eventEnd + 1 == EVENT_BUFFER_CAPACITY ? 0 : eventEnd + 1;
			eventBufferSize++;
		}
		else
		{
			SPRL_CORE_WARN("Event buffer overflow!");
		}
	}

	void Client::pushLayer(Layer* layer)
	{
		uint16_t i;
		if (pushLayerSize + 1 > pushLayerCapacity)
		{
			pushLayerCapacity += INITIAL_STACK_CAPACITY;
			Layer** t = (Layer**)malloc(sizeof(Layer*) * pushLayerCapacity);
			for (i = 0; i < pushLayerSize; i++)
			{
				t[i] = pushLayerBuffer[i];
			}
			t[pushLayerSize] = layer;
			pushLayerSize++;
			free(pushLayerBuffer);
			pushLayerBuffer = t;
		}
		else
		{
			pushLayerBuffer[pushLayerSize] = layer;
			pushLayerSize++;
		}
	}

	void Client::popLayer()
	{
		popLayerBuffer++;
	}

	void Client::pushOverlay(Layer* layer)
	{
		uint16_t i;
		if (pushOverlaySize + 1 > pushOverlayCapacity)
		{
			pushOverlayCapacity += INITIAL_STACK_CAPACITY;
			Layer** t = (Layer**)malloc(sizeof(Layer*) * pushOverlayCapacity);
			for (i = 0; i < pushOverlaySize; i++)
			{
				t[i] = pushOverlayBuffer[i];
			}
			t[pushOverlaySize] = layer;
			pushOverlaySize++;
			free(pushOverlayBuffer);
			pushOverlayBuffer = t;
		}
		else
		{
			pushOverlayBuffer[pushOverlaySize] = layer;
			pushOverlaySize++;
		}
	}

	void Client::popOverlay()
	{
		popOverlayBuffer++;
	}

	void Client::clearLayers()
	{
		popLayerBuffer = nLayers;
	}

	void Client::run()
	{
//#define TEST
#ifdef TEST
		Test app;

		try {
			app.run();
		}
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
#else

		while (running)
		{
			uint16_t i;
			//TODO: main loop

			window->tick();

			//TODO: insert resize window event

			while (eventBufferSize > 0)
			{
				SPRL_CORE_INFO("Window resize event: ({0}, {1}) - Index = {2}", eventBuffer[eventStart].x, eventBuffer[eventStart].y, eventStart);
				//TODO: handle events by looping through layers

				//TODO: handle missed events such as window close
				if (eventBuffer[eventStart].type == EventType::WindowClose)
				{
					running = false;
				}

				eventStart = eventStart + 1 == EVENT_BUFFER_CAPACITY ? 0 : eventStart + 1;
				eventBufferSize--;
			}

			if (popLayerBuffer > 0)
			{
				//TODO: pop layers

				popLayerBuffer = 0;
			}

			if (popOverlayBuffer > 0)
			{
				//TODO: pop overlays

				popOverlayBuffer = 0;
			}

			if (pushLayerSize > 0)
			{
				//TODO: push layers
				
				pushLayerSize = 0;
			}

			if (pushOverlaySize > 0)
			{
				//TODO: push overlays

				pushOverlaySize = 0;
			}
		}

		delete window; //TODO: move to destructor
#endif 
	}
}