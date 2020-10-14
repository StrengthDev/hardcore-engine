#pragma once

#ifndef NDEBUG
#define LOGINIT
#define LOGSHUTDOWN
#else
#define LOGINIT Spiral::logInit()
#define LOGSHUTDOWN Spiral::logShutdown()
#endif


extern Spiral::Client* Spiral::start();

int main(int argc, char** argv)
{
	LOGINIT;
	auto client = Spiral::start();
	client->init();
	client->pushInitialLayers();
	client->run();
	delete client;
	LOGSHUTDOWN;
}