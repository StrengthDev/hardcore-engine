#include <pch.hpp>

#include <debug/log_internal.hpp>
#include <debug/ansi_utility.hpp>

#ifdef _MSC_VER
#include <Windows.h>
#endif // _MSC_VER

namespace ENGINE_NAMESPACE
{
	namespace log
	{
#ifdef _MSC_VER
		static HANDLE out_handle;
		static DWORD default_out_mode;

		void init()
		{
#ifndef NDEBUG
			DWORD out_mode = 0;
			out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

			if (out_handle == INVALID_HANDLE_VALUE)
			{
				CRASH("Failed to get output handle.", GetLastError());
			}

			if (!GetConsoleMode(out_handle, &out_mode))
			{
				CRASH("Failed to get output mode.", GetLastError());
			}

			default_out_mode = out_mode;
			out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

			if (!SetConsoleMode(out_handle, out_mode))
			{
				CRASH("Failed to set output mode.", GetLastError());
			}
#endif // !NDEBUG
		}

		void shutdown()
		{
#ifndef NDEBUG
			std::cout << ANSI_RESET;

			if (!SetConsoleMode(out_handle, default_out_mode))
			{
				CRASH("Failed to reset output mode.", GetLastError());
			}
#endif // !NDEBUG
		}

		inline std::tm local_time(std::time_t* time)
		{
			std::tm ret = {};
			localtime_s(&ret, time);
			return ret;
		}
#else
		void init() {}
		void shutdown() {}

		inline std::tm local_time(std::time_t* time)
		{
			std::tm ret = {};
			localtime_r(time, &ret);
			return ret;
		}
#endif // _MSC_VER

		const char* empty_string = "";

		const char* log_type_strings[] = {
			"(-TRACE--) ",
			"(-DEBUG--) ",
			"(--INFO--) ",
			"(WARNING-) ",
			"(-ERROR--) ",
			"(CRITICAL) "
		};

		const char* caller_strings[] = {
			"ENGINE:  ",
			"CLIENT:  "
		};

		flag_t log_mask_flags = TRACE_BIT ^ 0xff;// 0xff;
		flag_t log_format_flags = CALLER_BIT;// 0xff;

		flag_t log_file_mask_flags = 0xff;
		flag_t log_file_format_flags = 0xff;

		void set_log_mask_flags(flag_t flags)
		{
			log_mask_flags = flags;
		}

		void set_log_format_flags(flag_t flags)
		{
			log_format_flags = flags;
		}
		//TODO: logging into a file

		inline void timestamp(std::ostream& stream)
		{
			const unsigned int buffer_size = 12;
			char time_s[buffer_size];
			std::time_t t = std::time(0);
			std::tm local_t = local_time(&t);
			strftime(time_s, buffer_size, "[%X] ", &local_t);
			stream << time_s;
		}

		inline void print_log(const char* message, const unsigned int caller_idx, const unsigned int type_idx)
		{
			if (log_format_flags & TIMESTAMP_BIT)
				timestamp(std::cout);

			std::cout
				<< (log_format_flags & EXPLICIT_TYPE_BIT ? log_type_strings[type_idx] : empty_string)
				<< (log_format_flags & CALLER_BIT ? caller_strings[caller_idx] : empty_string)
				<< message;
		}

		inline void log(const char* message, const unsigned int caller_idx, const unsigned int type_idx, const setting_t arg)
		{
			std::cout << ANSI_SETTING(arg);
			print_log(message, caller_idx, type_idx);
			std::cout << ANSI_RESET << std::endl;
		}

		inline void log(const char* message, const unsigned int caller_idx, const unsigned int type_idx, const setting_t arg_0, const setting_t arg_1, const setting_t arg_2)
		{
			std::cout << ANSI_SETTINGS(arg_0, arg_1, arg_2);
			print_log(message, caller_idx, type_idx);
			std::cout << ANSI_RESET << std::endl;
		}
		
		void trace(const char* message)
		{
			if(log_mask_flags & TRACE_BIT)
				log(message, 1, 0, WHITE);
		}

		void debug(const char* message)
		{
			if (log_mask_flags & DEBUG_BIT)
				log(message, 1, 1, CYAN);
		}

		void info(const char* message)
		{
			if (log_mask_flags & INFO_BIT)
				log(message, 1, 2, GREEN);
		}

		void warn(const char* message)
		{
			if (log_mask_flags & WARN_BIT)
				log(message, 1, 3, YELLOW, BOLD, DEFAULT_FONT);
		}

		void error(const char* message)
		{
			if (log_mask_flags & ERROR_BIT)
				log(message, 1, 4, RED, BOLD, DEFAULT_FONT);
		}

		void crit(const char* message)
		{
			if (log_mask_flags & CRIT_BIT)
				log(message, 1, 5, BRIGHT_MAGENTA, BOLD, DEFAULT_FONT);
		}
	}

	namespace internal
	{
		namespace log
		{
			void trace(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::TRACE_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 0, ENGINE_NAMESPACE::log::WHITE);
			}

			void debug(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::DEBUG_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 1, ENGINE_NAMESPACE::log::CYAN);
			}

			void info(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::INFO_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 2, ENGINE_NAMESPACE::log::GREEN);
			}

			void warn(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::WARN_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 3, ENGINE_NAMESPACE::log::YELLOW, ENGINE_NAMESPACE::log::BOLD, 
						ENGINE_NAMESPACE::log::DEFAULT_FONT);
			}

			void error(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::ERROR_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 4, ENGINE_NAMESPACE::log::RED, ENGINE_NAMESPACE::log::BOLD, 
						ENGINE_NAMESPACE::log::DEFAULT_FONT);
			}

			void crit(const char* message)
			{
				if (ENGINE_NAMESPACE::log::log_mask_flags & ENGINE_NAMESPACE::log::CRIT_BIT)
					ENGINE_NAMESPACE::log::log(message, 0, 5, ENGINE_NAMESPACE::log::BRIGHT_MAGENTA, 
						ENGINE_NAMESPACE::log::BOLD, ENGINE_NAMESPACE::log::DEFAULT_FONT);
			}
		}
	}
}
