#include <pch.hpp>

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

#include <core/entry_point_internal.hpp>

#include <parallel/thread_manager.hpp>
#include <debug/log_internal.hpp>

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

namespace ENGINE_NAMESPACE
{
	namespace internal
	{
		void init()
		{
			ENABLE_MEM_CHECKING;
			parallel::launch_threads();
			ENGINE_NAMESPACE::log::init();
		}

		void terminate()
		{
			parallel::terminate_threads();
			ENGINE_NAMESPACE::log::shutdown();
			parallel::logger_wait();
			END;
		}
	}
}