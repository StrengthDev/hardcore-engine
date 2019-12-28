#include <Spiral.hpp>

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient()
	{
		SPRL_INFO("Initializing Client");
	}

	~ExampleClient()
	{

	}
};

Spiral::Client* Spiral::Start()
{
	return new ExampleClient();
}