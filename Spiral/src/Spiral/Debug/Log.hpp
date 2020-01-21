#pragma once

#include "Spiral/Core/Core.hpp"

#include <spdlog/spdlog.h>

namespace Spiral
{
	SPIRAL_API void logInit();
	SPIRAL_API void logShutdown();
	
	SPIRAL_API inline std::shared_ptr<spdlog::logger>& coreLog();
	SPIRAL_API inline std::shared_ptr<spdlog::logger>& clientLog();
}

#ifndef NDEBUG
#define SPRL_CORE_TRACE(...)
#define SPRL_CORE_DEBUG(...)
#define SPRL_CORE_INFO(...)	
#define SPRL_CORE_WARN(...)	
#define SPRL_CORE_ERROR(...)
#define SPRL_CORE_CRIT(...)	

#define SPRL_TRACE(...)		
#define SPRL_DEBUG(...)		
#define SPRL_INFO(...)		
#define SPRL_WARN(...)		
#define SPRL_ERROR(...)		
#define SPRL_CRIT(...)		
#else
#define SPRL_CORE_TRACE(...)	Spiral::coreLog()->trace(__VA_ARGS__)
#define SPRL_CORE_DEBUG(...)	Spiral::coreLog()->debug(__VA_ARGS__)
#define SPRL_CORE_INFO(...)		Spiral::coreLog()->info(__VA_ARGS__)
#define SPRL_CORE_WARN(...)		Spiral::coreLog()->warn(__VA_ARGS__)
#define SPRL_CORE_ERROR(...)	Spiral::coreLog()->error(__VA_ARGS__)
#define SPRL_CORE_CRIT(...)		Spiral::coreLog()->critical(__VA_ARGS__)

#define SPRL_TRACE(...)			Spiral::clientLog()->trace(__VA_ARGS__)
#define SPRL_DEBUG(...)			Spiral::clientLog()->debug(__VA_ARGS__)
#define SPRL_INFO(...)			Spiral::clientLog()->info(__VA_ARGS__)
#define SPRL_WARN(...)			Spiral::clientLog()->warn(__VA_ARGS__)
#define SPRL_ERROR(...)			Spiral::clientLog()->error(__VA_ARGS__)
#define SPRL_CRIT(...)			Spiral::clientLog()->critical(__VA_ARGS__)
#endif