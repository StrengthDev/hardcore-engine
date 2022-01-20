#pragma once

#include "log.hpp"

namespace Spiral
{
	namespace log
	{
		SPIRAL_API void init();
		SPIRAL_API void shutdown();
	}

	namespace internal
	{
		namespace log
		{
			void trace(const char* message);
			template<typename... Types>
			inline void tracef(const std::string& format, Types... args) { trace(Spiral::log::str_format(format, args...).c_str()); }

			void debug(const char* message);
			template<typename... Types>
			inline void debugf(const std::string& format, Types... args) { debug(Spiral::log::str_format(format, args...).c_str()); }

			void info(const char* message);
			template<typename... Types>
			inline void infof(const std::string& format, Types... args) { info(Spiral::log::str_format(format, args...).c_str()); }

			void warn(const char* message);
			template<typename... Types>
			inline void warnf(const std::string& format, Types... args) { warn(Spiral::log::str_format(format, args...).c_str()); }

			void error(const char* message);
			template<typename... Types>
			inline void errorf(const std::string& format, Types... args) { error(Spiral::log::str_format(format, args...).c_str()); }

			void crit(const char* message);
			template<typename... Types>
			inline void critf(const std::string& format, Types... args) { crit(Spiral::log::str_format(format, args...).c_str()); }
		}
	}
}

#ifndef NDEBUG
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
#define LOG_INTERNAL_TRACE(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::trace(str_stream.str().c_str()); }
#define LOG_INTERNAL_DEBUG(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::debug(str_stream.str().c_str()); }
#define LOG_INTERNAL_INFO(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::info(str_stream.str().c_str()); }
#define LOG_INTERNAL_WARN(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::warn(str_stream.str().c_str()); }
#define LOG_INTERNAL_ERROR(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::error(str_stream.str().c_str()); }
#define LOG_INTERNAL_CRIT(message)	{ std::stringstream str_stream; str_stream << message; Spiral::internal::log::crit(str_stream.str().c_str()); }

#define LOGF_INTERNAL_TRACE(...)	Spiral::internal::log::tracef(__VA_ARGS__);
#define LOGF_INTERNAL_DEBUG(...)	Spiral::internal::log::debugf(__VA_ARGS__);
#define LOGF_INTERNAL_INFO(...)		Spiral::internal::log::infof(__VA_ARGS__);
#define LOGF_INTERNAL_WARN(...)		Spiral::internal::log::warnf(__VA_ARGS__);
#define LOGF_INTERNAL_ERROR(...)	Spiral::internal::log::errorf(__VA_ARGS__);
#define LOGF_INTERNAL_CRIT(...)		Spiral::internal::log::critf(__VA_ARGS__);
#endif // NDEBUG
