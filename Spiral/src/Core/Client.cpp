#include "pch.hpp"

#include <Spiral/Core/Client.hpp>

namespace Spiral
{
	ECProperties Client::engineProperties = {
		SPIRAL_NAME,
		SPIRAL_MAJOR_VERSION,
		SPIRAL_MINOR_VERSION,
		SPIRAL_PATCH_VERSION
	};

	Client* Client::instance = nullptr;

	Client::Client() //TODO: might want to consider giving an absolute size to the layerstack and move it to the stack memory space instead of heap
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
		window->setSizeCallback(std::bind(&Client::pushWindowSize, this, std::placeholders::_1, std::placeholders::_2));

		window->getDimensions(&windowWidth, &windowHeight);
		windowSizeChanged = false;

		setProperties("Spiral Application", 1, 0, 0);

		eventBufferSize = 0;
		eventStart = 0;
		eventEnd = 0;
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

		free(layerStack);
		free(pushLayerBuffer);
		free(pushOverlayBuffer);
	}

	void Client::init()
	{
		//SPRL_CORE_TRACE(properties.name);
		renderer = Renderer::init(engineProperties, properties);
	}

	void Client::setProperties(const char *name, unsigned int majorVersion, unsigned int minorVersion, unsigned int patchVersion)
	{
		properties.name = name;
		properties.major = majorVersion;
		properties.minor = minorVersion;
		properties.patch = patchVersion;
	}

	static std::mutex eventMutex;

	void Client::pushEvent(Event e)
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
			SPRL_CORE_WARN("Event buffer overflow!");
		}
	}

	void Client::pushWindowSize(int width, int height)
	{
		windowWidth = width;
		windowHeight = height;
		windowSizeChanged = true;
	}

	void Client::pushLayer(Layer *layer)
	{
		index_t i;
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

	void Client::pushOverlay(Layer *layer)
	{
		index_t i;
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
		index_t i, n;
		Layer** t;
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
				pushEvent(Event::windowResize(windowWidth, windowHeight));
				windowSizeChanged = false;
			}
			//TODO: use function_name, var_name, struct_name, name_space, ClassName, TemplateParam -- add include directory, rename files with no capital letters
			while (eventBufferSize > 0) //TODO: replace goto with function
			{
				for (i = nLayers + nOverlays - 1; i != 65535U; i--) //Kinda assuming the layer stack will never get as big as 65535, but it shouldnt
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
				for (i = nLayers; i < nLayers + nOverlays; i++)
				{
					layerStack[i] = layerStack[i + popLayerBuffer];
				}
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
					for (i = 0; i < nLayers + nOverlays; i++)
					{
						t[i] = layerStack[i];
					}
					free(layerStack);
					layerStack = t;
				}

				if (pushLayerSize)
				{
					for (i = nLayers + nOverlays + pushLayerSize - 1; i > nLayers + pushLayerSize - 1; i--)
					{
						layerStack[i] = layerStack[i - pushLayerSize];
					}
					for (i = nLayers; i < nLayers + pushLayerSize; i++)
					{
						layerStack[i] = pushLayerBuffer[i - nLayers];
					}
					nLayers += pushLayerSize;
					pushLayerSize = 0;
				}

				if (pushOverlaySize)
				{
					n = nLayers + nOverlays;
					for (i = n; i < n + pushOverlaySize; i++)
					{
						layerStack[i] = pushOverlayBuffer[i - n];
					}
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