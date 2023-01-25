#pragma once

#include <core/core.hpp>

#include <debug/log.hpp>

namespace ENGINE_NAMESPACE
{
	namespace log
	{
		ENGINE_API void init();
		ENGINE_API void shutdown();
	}

	namespace internal
	{
		namespace log
		{
			void trace(const char* message);
			template<typename... Types>
			inline void tracef(const std::string& format, Types... args)
			{
				trace(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}

			void debug(const char* message);
			template<typename... Types>
			inline void debugf(const std::string& format, Types... args)
			{
				debug(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}

			void info(const char* message);
			template<typename... Types>
			inline void infof(const std::string& format, Types... args)
			{
				info(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}

			void warn(const char* message);
			template<typename... Types>
			inline void warnf(const std::string& format, Types... args)
			{
				warn(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}

			void error(const char* message);
			template<typename... Types>
			inline void errorf(const std::string& format, Types... args)
			{
				error(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}

			void crit(const char* message);
			template<typename... Types>
			inline void critf(const std::string& format, Types... args)
			{
				crit(ENGINE_NAMESPACE::log::str_format(format, args...).c_str());
			}
		}
	}
}

#ifdef NDEBUG
#define LOG_INTERNAL_TRACE(message)
#define LOG_INTERNAL_DEBUG(message)
#define LOG_INTERNAL_INFO(message)
#define LOG_INTERNAL_WARN(message)
#define LOG_INTERNAL_ERROR(message)
#define LOG_INTERNAL_CRIT(message)

#define LOGF_INTERNAL_TRACE(...)
#define LOGF_INTERNAL_DEBUG(...)
#define LOGF_INTERNAL_INFO(...)
#define LOGF_INTERNAL_WARN(...)
#define LOGF_INTERNAL_ERROR(...)
#define LOGF_INTERNAL_CRIT(...)
#else
#define LOG_INTERNAL_TRACE(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::trace(str_stream.str().c_str()); }
#define LOG_INTERNAL_DEBUG(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::debug(str_stream.str().c_str()); }
#define LOG_INTERNAL_INFO(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::info(str_stream.str().c_str()); }
#define LOG_INTERNAL_WARN(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::warn(str_stream.str().c_str()); }
#define LOG_INTERNAL_ERROR(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::error(str_stream.str().c_str()); }
#define LOG_INTERNAL_CRIT(message)	{ std::stringstream str_stream; str_stream << message; ENGINE_NAMESPACE::internal::log::crit(str_stream.str().c_str()); }

#define LOGF_INTERNAL_TRACE(...)	ENGINE_NAMESPACE::internal::log::tracef(__VA_ARGS__);
#define LOGF_INTERNAL_DEBUG(...)	ENGINE_NAMESPACE::internal::log::debugf(__VA_ARGS__);
#define LOGF_INTERNAL_INFO(...)		ENGINE_NAMESPACE::internal::log::infof(__VA_ARGS__);
#define LOGF_INTERNAL_WARN(...)		ENGINE_NAMESPACE::internal::log::warnf(__VA_ARGS__);
#define LOGF_INTERNAL_ERROR(...)	ENGINE_NAMESPACE::internal::log::errorf(__VA_ARGS__);
#define LOGF_INTERNAL_CRIT(...)		ENGINE_NAMESPACE::internal::log::critf(__VA_ARGS__);
#endif // NDEBUG
