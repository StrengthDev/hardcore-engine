#pragma once

#ifndef NDEBUG

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //TODO: adapt to be platform independent

#define LOGINIT
#define LOGSHUTDOWN
#else
#define LOGINIT Spiral::Log::init()
#define LOGSHUTDOWN Spiral::Log::shutdown()
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