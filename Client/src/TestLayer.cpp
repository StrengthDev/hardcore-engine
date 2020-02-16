#include "TestLayer.hpp"

TestLayer::TestLayer()
{
	start = std::chrono::steady_clock::now();
	timec = 0;
	framec = 0;
}

TestLayer::~TestLayer()
{

}

void TestLayer::tick()
{
	std::chrono::nanoseconds duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start);
	timec += duration.count();
	if (timec > 1000000000)
	{
		timec -= 1000000000;
		SPRL_TRACE("Frame count: ({0})", framec);
		framec = 0;
	}
	framec++;
	start = std::chrono::steady_clock::now();
}

bool TestLayer::handleEvent(const Spiral::Event &e)
{
	return false;
}