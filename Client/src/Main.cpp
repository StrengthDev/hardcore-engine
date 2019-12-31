#include <Spiral.hpp>
#include <Spiral/Core/EntryPoint.hpp>

#include "TestLayer.hpp"

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient()
	{
		pushLayer(new TestLayer(), 0);
	}

	~ExampleClient()
	{

	}
};

Spiral::Client* Spiral::start()
{
	return new ExampleClient();
}