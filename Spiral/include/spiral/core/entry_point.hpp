#pragma once


#ifdef NDEBUG

#ifdef _MSC_VER
#else
#endif // _MSC_VER

#else

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#endif // _MSC_VER

#endif // NDEBUG

#include <spiral/core/core.hpp>
#include <spiral/core/client.hpp>
#include <spiral/debug/log_internal.hpp>

#ifdef NDEBUG

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //TODO: adapt to be platform independent
#define ENABLE_MEM_CHECKING
#else
#endif // _MSC_VER
#define END 

#else

#ifdef _MSC_VER
#define ENABLE_MEM_CHECKING _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF)
#define END system("pause")
#else
#endif // _MSC_VER

#endif // NDEBUG


extern ENGINE_NAMESPACE::client* ENGINE_NAMESPACE::start();

int main(int argc, char** argv)
{
	ENABLE_MEM_CHECKING;
	ENGINE_NAMESPACE::log::init();
	auto client = ENGINE_NAMESPACE::start();
	client->run();
	delete client;
	ENGINE_NAMESPACE::log::shutdown();
	END;
}

#undef ENABLE_MEM_CHECKING
#undef END
