#pragma once

#include <spiral/core/core.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Spiral //TODO: make core logger invisible to the client
{
	namespace Log
	{
		SPIRAL_API void init();
		SPIRAL_API void shutdown();
	
		SPIRAL_API std::shared_ptr<spdlog::logger>& core();
		SPIRAL_API std::shared_ptr<spdlog::logger>& client();
		/*
		template<typename T>
		SPIRAL_API void coreTrace(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreTrace(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void coreDebug(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreDebug(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void coreInfo(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreInfo(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void coreWarn(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreWarn(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void coreError(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreError(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void coreCritical(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void coreCritical(const FormatString& fmt, const Args &...args);



		template<typename T>
		SPIRAL_API void clientTrace(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientTrace(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void clientDebug(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientDebug(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void clientInfo(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientInfo(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void clientWarn(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientWarn(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void clientError(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientError(const FormatString& fmt, const Args &...args);

		template<typename T>
		SPIRAL_API void clientCritical(const T& msg);
		template<typename FormatString, typename... Args>
		SPIRAL_API void clientCritical(const FormatString& fmt, const Args &...args);
		*/
	}
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

#define SPRL_INIT_CLIENT_LOGGER
#else
/*
#define SPRL_CORE_TRACE(...)	Spiral::Log::coreTrace(__VA_ARGS__)
#define SPRL_CORE_DEBUG(...)	Spiral::Log::coreDebug(__VA_ARGS__)
#define SPRL_CORE_INFO(...)		Spiral::Log::coreInfo(__VA_ARGS__)
#define SPRL_CORE_WARN(...)		Spiral::Log::coreWarn(__VA_ARGS__)
#define SPRL_CORE_ERROR(...)	Spiral::Log::coreError(__VA_ARGS__)
#define SPRL_CORE_CRIT(...)		Spiral::Log::coreCritical(__VA_ARGS__)

#define SPRL_TRACE(...)			Spiral::Log::clientTrace(__VA_ARGS__)
#define SPRL_DEBUG(...)			Spiral::Log::clientDebug(__VA_ARGS__)
#define SPRL_INFO(...)			Spiral::Log::clientInfo(__VA_ARGS__)
#define SPRL_WARN(...)			Spiral::Log::clientWarn(__VA_ARGS__)
#define SPRL_ERROR(...)			Spiral::Log::clientError(__VA_ARGS__)
#define SPRL_CRIT(...)			Spiral::Log::clientCritical(__VA_ARGS__)
*/

#define SPRL_CORE_TRACE(...)	Spiral::Log::core()->trace(__VA_ARGS__)
#define SPRL_CORE_DEBUG(...)	Spiral::Log::core()->debug(__VA_ARGS__)
#define SPRL_CORE_INFO(...)		Spiral::Log::core()->info(__VA_ARGS__)
#define SPRL_CORE_WARN(...)		Spiral::Log::core()->warn(__VA_ARGS__)
#define SPRL_CORE_ERROR(...)	Spiral::Log::core()->error(__VA_ARGS__)
#define SPRL_CORE_CRIT(...)		Spiral::Log::core()->critical(__VA_ARGS__)

#define SPRL_TRACE(...)			Spiral::Log::client()->trace(__VA_ARGS__)
#define SPRL_DEBUG(...)			Spiral::Log::client()->debug(__VA_ARGS__)
#define SPRL_INFO(...)			Spiral::Log::client()->info(__VA_ARGS__)
#define SPRL_WARN(...)			Spiral::Log::client()->warn(__VA_ARGS__)
#define SPRL_ERROR(...)			Spiral::Log::client()->error(__VA_ARGS__)
#define SPRL_CRIT(...)			Spiral::Log::client()->critical(__VA_ARGS__)

#define SPRL_INIT_CLIENT_LOGGER { spdlog::set_pattern("%^[%T] %n: %v%$"); Spiral::Log::client() = spdlog::stdout_color_mt("CLIENT"); Spiral::Log::client()->set_level(spdlog::level::trace); Spiral::Log::client()->debug("Logger initiated."); }
#endif