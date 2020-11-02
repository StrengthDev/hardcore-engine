#include "pch.hpp"

#include "Log.hpp"
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

		/*
		template<typename T>
		void coreTrace(const T& msg)
		{
			coreLogger->trace(msg);
		}

		template<typename FormatString, typename... Args>
		void coreTrace(const FormatString& fmt, const Args &...args)
		{
			coreLogger->trace(fmt, args...);
		}

		template<typename T>
		void coreDebug(const T& msg)
		{
			coreLogger->debug(msg);
		}

		template<typename FormatString, typename... Args>
		void coreDebug(const FormatString& fmt, const Args &...args)
		{
			coreLogger->debug(fmt, args...);
		}

		template<typename T>
		void coreInfo(const T& msg)
		{
			coreLogger->info(msg);
		}

		template<typename FormatString, typename... Args>
		void coreInfo(const FormatString& fmt, const Args &...args)
		{
			coreLogger->info(fmt, args...);
		}

		template<typename T>
		void coreWarn(const T& msg)
		{
			coreLogger->warn(msg);
		}

		template<typename FormatString, typename... Args>
		void coreWarn(const FormatString& fmt, const Args &...args)
		{
			coreLogger->warn(fmt, args...);
		}

		template<typename T>
		void coreError(const T& msg)
		{
			coreLogger->error(msg);
		}

		template<typename FormatString, typename... Args>
		void coreError(const FormatString& fmt, const Args &...args)
		{
			coreLogger->error(fmt, args...);
		}

		template<typename T>
		void coreCritical(const T& msg)
		{
			coreLogger->critical(msg);
		}

		template<typename FormatString, typename... Args>
		void coreCritical(const FormatString& fmt, const Args &...args)
		{
			coreLogger->critical(fmt, args...);
		}


		template<typename T>
		void clientTrace(const T& msg)
		{
			clientLogger->trace(msg);
		}

		template<typename FormatString, typename... Args>
		void clientTrace(const FormatString& fmt, const Args &...args)
		{
			clientLogger->trace(fmt, args...);
		}

		template<typename T>
		void clientDebug(const T& msg)
		{
			clientLogger->debug(msg);
		}

		template<typename FormatString, typename... Args>
		void clientDebug(const FormatString& fmt, const Args &...args)
		{
			clientLogger->debug(fmt, args...);
		}

		template<typename T>
		void clientInfo(const T& msg)
		{
			clientLogger->info(msg);
		}

		template<typename FormatString, typename... Args>
		void clientInfo(const FormatString& fmt, const Args &...args)
		{
			clientLogger->info(fmt, args...);
		}

		template<typename T>
		void clientWarn(const T& msg)
		{
			clientLogger->warn(msg);
		}

		template<typename FormatString, typename... Args>
		void clientWarn(const FormatString& fmt, const Args &...args)
		{
			clientLogger->warn(fmt, args...);
		}

		template<typename T>
		void clientError(const T& msg)
		{
			clientLogger->error(msg);
		}

		template<typename FormatString, typename... Args>
		void clientError(const FormatString& fmt, const Args &...args)
		{
			clientLogger->error(fmt, args...);
		}

		template<typename T>
		void clientCritical(const T& msg)
		{
			clientLogger->critical(msg);
		}

		template<typename FormatString, typename... Args>
		void clientCritical(const FormatString& fmt, const Args &...args)
		{
			clientLogger->critical(fmt, args...);
		}
		*/
		
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
