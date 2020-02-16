#include "pch.hpp"

#include "Log.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Spiral
{
	static std::shared_ptr<spdlog::logger> coreLogger;
	static std::shared_ptr<spdlog::logger> clientLogger;

	void logInit()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
	
		coreLogger = spdlog::stdout_color_mt("SPIRAL");
		coreLogger->set_level(spdlog::level::trace);
	
		clientLogger = spdlog::stdout_color_mt("CLIENT");
		clientLogger->set_level(spdlog::level::trace);
	}

	void logShutdown()
	{
		spdlog::shutdown();
	}
	
	std::shared_ptr<spdlog::logger>& coreLog()
	{
		return coreLogger;
	}
	
	std::shared_ptr<spdlog::logger>& clientLog()
	{
		return clientLogger;
	}
}
