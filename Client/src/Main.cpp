#include <Spiral.hpp>
#include <Spiral/Core/EntryPoint.hpp>

#include "TestLayer.hpp"
#include "ELLayer.hpp"

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient()
	{
		pushLayer(new TestLayer());
		pushLayer(new ELLayer());

		getWindow().setTitle("Chad Engine");

		const char* files[2] =
		{
			"assets/icons/icon1.png",
			"assets/icons/icon2.png"
		};

		getWindow().setIcon(files, 2);
		//setProperties("Debug Program", 1, 0, 0);
	}

	~ExampleClient()
	{
		
	}
};

Spiral::Client* Spiral::start()
{
	return new ExampleClient();
}