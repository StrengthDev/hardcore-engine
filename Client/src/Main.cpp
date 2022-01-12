#include <spiral.hpp>
#include <Spiral/Core/EntryPoint.hpp>

#include "TestLayer.hpp"
#include "ELLayer.hpp"

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient()
	{
		//TODO: this is stupid, change to loose functions instead of class static methods
		getWindow().setTitle("Chad Engine");

		const char* files[2] =
		{
			"resources/icons/icon1.png",
			"resources/icons/icon2.png"
		};

		getWindow().setIcon(files, 2);
		//setProperties("Debug Program", 1, 0, 0);
		SPRL_INIT_CLIENT_LOGGER;
	}

	~ExampleClient()
	{
		
	}

	void pushInitialLayers()
	{
		pushLayer(new TestLayer());
		pushLayer(new ELLayer());
	}
};

Spiral::Client* Spiral::start()
{
	return new ExampleClient();
}