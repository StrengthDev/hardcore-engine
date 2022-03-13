#pragma once

#include <spiral.hpp>
#include <spiral/core/client.hpp>
#include <spiral/debug/log_internal.hpp>

#ifndef NDEBUG

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //TODO: adapt to be platform independent
#endif // _MSC_VER

#else

#endif // NDEBUG


extern Spiral::client* Spiral::start();

int main(int argc, char** argv)
{
	Spiral::log::init();
	auto client = Spiral::start();
	client->run();
	delete client;
	Spiral::log::shutdown();
}