#pragma once

#ifndef NDEBUG
#define LOGINIT Spiral::logInit()
#define LOGSHUTDOWN Spiral::logShutdown()
#else
#define LOGINIT
#define LOGSHUTDOWN
#endif


extern Spiral::Client* Spiral::start();

int main(int argc, char** argv)
{
	LOGINIT;
	auto client = Spiral::start();
	client->run();
	delete client;
	LOGSHUTDOWN;
}