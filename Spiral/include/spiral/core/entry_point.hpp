#pragma once

#include <spiral.hpp>
#include <spiral/core/client.hpp>
#include <spiral/debug/log_internal.hpp>

#ifndef NDEBUG

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //TODO: adapt to be platform independent
#endif // _MSC_VER

#define LOGINIT
#define LOGSHUTDOWN
#else
#define LOGINIT Spiral::log::init()
#define LOGSHUTDOWN Spiral::log::shutdown()
#endif // NDEBUG


extern Spiral::client* Spiral::start();

int main(int argc, char** argv)
{
	LOGINIT;
	auto client = Spiral::start();
	client->run();
	delete client;
	LOGSHUTDOWN;
}