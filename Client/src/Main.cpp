#include <Spiral.hpp>

class Client : public Spiral::Client
{
public:
	Client()
	{

	}

	~Client()
	{

	}
};

Spiral::Client* Spiral::Start()
{
	return new Client();
}