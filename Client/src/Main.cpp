#include <Spiral.hpp>

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient()
	{
		
	}

	~ExampleClient()
	{

	}
};

Spiral::Client* Spiral::Start()
{
	return new ExampleClient();
}