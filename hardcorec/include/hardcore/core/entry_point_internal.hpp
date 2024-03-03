#pragma once

#include "core.hpp"

namespace ENGINE_NAMESPACE
{
	namespace internal
	{
		ENGINE_API void init();
		ENGINE_API void terminate();
		ENGINE_API void exception_crash(const std::exception& e);
	}
}
