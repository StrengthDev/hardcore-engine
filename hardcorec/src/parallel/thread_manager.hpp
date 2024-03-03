#pragma once

#include <mutex>
#include <condition_variable>
#include <new>
#include <functional>
#include <exception>

#include <core/core.hpp>

namespace ENGINE_NAMESPACE
{
	namespace parallel
	{
		void launch_threads();
		void terminate_threads();

		void logger_wait();
	}
}
