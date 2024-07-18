#pragma once

#include <iostream>
#include <sstream>
#include <array>

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE
{
	namespace log
	{
		using flag_t = uint8_t;
		
		enum log_mask_flag_bits
		{
			TRACE_BIT =	BIT(0),
			DEBUG_BIT =	BIT(1),
			INFO_BIT =	BIT(2),
			WARN_BIT =	BIT(3),
			ERROR_BIT =	BIT(4),
			CRIT_BIT =	BIT(5)
		};

		enum log_format_flag_bits
		{
			TIMESTAMP_BIT =		BIT(0),
			CALLER_BIT =		BIT(1),
			EXPLICIT_TYPE_BIT =	BIT(2)
		};

		ENGINE_API void set_log_mask_flags(flag_t flags);
		ENGINE_API void set_log_format_flags(flag_t flags);

		template<std::size_t N, typename Type, typename... Types>
		inline void args_to_strings(std::array<std::string, N>& arr, Type arg, Types... args)
		{
			std::stringstream str_stream;
			str_stream << arg;
			arr[N - sizeof...(args) - 1] = str_stream.str();
			if constexpr (0 < sizeof...(args))
			{
				args_to_strings(arr, args...);
			}
		}

		//TODO: Optimize this, it is garbage
		template<typename... Types>
		inline std::string str_format(const std::string& format, Types... args)
		{
			const char* no_arg = "";

			if constexpr (sizeof...(Types) == 0)
			{
				return format;
			}
			else
			{
				std::array<std::string, sizeof...(args)> arg_strings;
				args_to_strings(arg_strings, args...);
				
				std::stringstream str_stream;
				std::size_t arg_n, t, pos = 0;
				std::size_t save_index = 0;
				std::string tmp;
				while ((pos = format.find('{', pos)) != std::string::npos)
				{
					t = format.find('}', pos);
					if (t == std::string::npos)
					{
						break;
					}

					tmp = format.substr(pos + 1, t - pos - 1);

					try
					{
						arg_n = std::stoull(tmp);
						str_stream << format.substr(save_index, pos - save_index);
						if (arg_n < sizeof...(args))
						{
							str_stream << arg_strings[arg_n];
						}
						else
						{
							str_stream << no_arg;
						}
						
						t++;
						save_index = t;
						pos = t;
					}
					catch (std::invalid_argument e)
					{
						pos++;
					}
				}
				if (save_index < format.length())
				{
					str_stream << format.substr(save_index, format.length() - save_index);
				}
				return str_stream.str();
			}
		}

		ENGINE_API void trace(const char* message);
		template<typename... Types>
		inline void tracef(const std::string& format, Types... args) { trace(str_format(format, args...).c_str()); }

		ENGINE_API void debug(const char* message);
		template<typename... Types>
		inline void debugf(const std::string& format, Types... args) { debug(str_format(format, args...).c_str()); }

		ENGINE_API void info(const char* message);
		template<typename... Types>
		inline void infof(const std::string& format, Types... args) { info(str_format(format, args...).c_str()); }

		ENGINE_API void warn(const char* message);
		template<typename... Types>
		inline void warnf(const std::string& format, Types... args) { warn(str_format(format, args...).c_str()); }

		ENGINE_API void error(const char* message);
		template<typename... Types>
		inline void errorf(const std::string& format, Types... args) { error(str_format(format, args...).c_str()); }

		ENGINE_API void crit(const char* message);
		template<typename... Types>
		inline void critf(const std::string& format, Types... args) { crit(str_format(format, args...).c_str()); }
	}
}

#ifdef NDEBUG
#define LOG_TRACE(message)
#define LOG_DEBUG(message)
#define LOG_INFO(message)
#define LOG_WARN(message)
#define LOG_ERROR(message)
#define LOG_CRIT(message)

#define LOGF_TRACE(...)
#define LOGF_DEBUG(...)
#define LOGF_INFO(...)
#define LOGF_WARN(...)
#define LOGF_ERROR(...)
#define LOGF_CRIT(...)
#else				
#define LOG_TRACE(message)	{ std::stringstream str_stream; str_stream << message; hc::log::trace(str_stream.str().c_str()); }
#define LOG_DEBUG(message)	{ std::stringstream str_stream; str_stream << message; hc::log::debug(str_stream.str().c_str()); }
#define LOG_INFO(message)	{ std::stringstream str_stream; str_stream << message; hc::log::info(str_stream.str().c_str()); }
#define LOG_WARN(message)	{ std::stringstream str_stream; str_stream << message; hc::log::warn(str_stream.str().c_str()); }
#define LOG_ERROR(message)	{ std::stringstream str_stream; str_stream << message; hc::log::error(str_stream.str().c_str()); }
#define LOG_CRIT(message)	{ std::stringstream str_stream; str_stream << message; hc::log::crit(str_stream.str().c_str()); }

#define LOGF_TRACE(...)	hc::log::tracef(__VA_ARGS__);
#define LOGF_DEBUG(...)	hc::log::debugf(__VA_ARGS__);
#define LOGF_INFO(...)	hc::log::infof(__VA_ARGS__);
#define LOGF_WARN(...)	hc::log::warnf(__VA_ARGS__);
#define LOGF_ERROR(...)	hc::log::errorf(__VA_ARGS__);
#define LOGF_CRIT(...)	hc::log::critf(__VA_ARGS__);
#endif // NDEBUG