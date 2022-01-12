#include <pch.hpp>

#include <spiral/debug/log.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Spiral
{
	namespace Log
	{
		static std::shared_ptr<spdlog::logger> coreLogger;
		static std::shared_ptr<spdlog::logger> clientLogger;

		void init()
		{
			spdlog::set_pattern("%^[%T] %n: %v%$");
	
			coreLogger = spdlog::stdout_color_mt("SPIRAL");
			coreLogger->set_level(spdlog::level::trace);
			coreLogger->debug("Logger initiated.");
			/*
			clientLogger = spdlog::stdout_color_mt("CLIENT");
			clientLogger->set_level(spdlog::level::trace);
			clientLogger->debug("Logger initiated.");
			*/
		}

		void shutdown()
		{
			spdlog::shutdown();
		}
		
		std::shared_ptr<spdlog::logger>& core()
		{
			return coreLogger;
		}
	
		std::shared_ptr<spdlog::logger>& client()
		{
			return clientLogger;
		}
		
	}
}
