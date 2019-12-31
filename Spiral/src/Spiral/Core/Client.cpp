#include "pch.hpp"

#include "Client.hpp"

namespace Spiral
{
	Client* Client::instance = nullptr;

	Client::Client()
	{
		instance = this;

		//layerstack = LayerStack(LAYER_TIERS);
	}

	Client::~Client()
	{

	}

	void Client::pushLayer(Layer* layer, uint32_t tier)
	{

	}

	void Client::popLayer(uint32_t tier)
	{

	}

	void Client::run()
	{
		Test app;

		try {
			app.run();
		}
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
}