#include "Client.hpp"

#include <iostream>
#include <stdexcept>

namespace Spiral
{
	Client::Client()
	{

	}

	Client::~Client()
	{

	}

	void Client::Run()
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