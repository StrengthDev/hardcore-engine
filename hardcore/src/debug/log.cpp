#include <pch.hpp>

#include <debug/log_internal.hpp>
#include <debug/ansi_utility.hpp>

#include <parallel/concurrent_queue.hpp>

#ifdef _MSC_VER
#include <Windows.h>
#endif // _MSC_VER

namespace ENGINE_NAMESPACE
{
	namespace log
	{
		class log_entry
		{
		public:
			inline log_entry(std::string message, std::string time, u8 caller_idx, u8 type_idx, 
				setting_t arg) : 
				message(message), time(time), caller_idx(caller_idx), type_idx(type_idx), multi_args(false), arg0(arg)
			{}

			inline log_entry(std::string message, std::string time, u8 caller_idx, u8 type_idx, 
				setting_t arg0, setting_t arg1, setting_t arg2) : 
				message(message), time(time), caller_idx(caller_idx), type_idx(type_idx),
				multi_args(true), arg0(arg0), arg1(arg1), arg2(arg2)
			{}

			std::string message;
			std::string time;

			u8 caller_idx;
			u8 type_idx;

			bool multi_args;
			setting_t arg0;
			setting_t arg1 = 0;
			setting_t arg2 = 0;
		};

		parallel::concurrent_queue<log_entry> queue;

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

		std::atomic<flag_t> log_mask_flags = TRACE_BIT ^ 0xff;// 0xff;
		std::atomic<flag_t> log_format_flags = CALLER_BIT;// 0xff;

		std::atomic<flag_t> log_file_mask_flags = 0xff;
		std::atomic<flag_t> log_file_format_flags = 0xff;

#ifdef _MSC_VER
		static HANDLE out_handle;
		static DWORD default_out_mode;

		void init()
		{
			std::ios_base::sync_with_stdio(false);
			std::setvbuf(stdout, nullptr, _IOFBF, KILOBYTES(4));

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
			queue.close();
			std::ios_base::sync_with_stdio(true);

#ifndef NDEBUG
			std::cout << ANSI_RESET;

			if (!SetConsoleMode(out_handle, default_out_mode))
			{
				CRASH("Failed to reset output mode.", GetLastError());
			}
#endif // !NDEBUG
		}

		void run()
		{
			try
			{
				while (true)
				{
					log_entry entry = queue.pop();

					std::stringstream stream;
					if (entry.multi_args)
						stream << ANSI_SETTINGS(entry.arg0, entry.arg1, entry.arg2);
					else
						stream << ANSI_SETTING(entry.arg0);
					stream
						<< entry.time
						<< (log_format_flags & EXPLICIT_TYPE_BIT ? log_type_strings[entry.type_idx] : "")
						<< (log_format_flags & CALLER_BIT ? caller_strings[entry.caller_idx] : "")
						<< entry.message
						<< ANSI_RESET << '\n';

					std::cout << stream.str();
					std::cout.flush();
				}
			}
			catch (const exception::closed_queue&)
			{}
		}

		void flush()
		{
			queue.wait_until_empty();
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

		void set_log_mask_flags(flag_t flags)
		{
			log_mask_flags = flags;
		}

		void set_log_format_flags(flag_t flags)
		{
			log_format_flags = flags;
		}
		//TODO: logging into a file

		inline std::string timestamp()
		{
			const unsigned int buffer_size = 12;
			char buf[buffer_size];
			std::time_t t = std::time(0);
			std::tm local_t = local_time(&t);
			strftime(buf, buffer_size, "[%X] ", &local_t);
			return std::string(buf);
		}

		inline void log(const char* message, u8 caller_idx, u8 type_idx, setting_t arg)
		{
			log_entry entry(message, log_format_flags & TIMESTAMP_BIT ? timestamp() : "", caller_idx, type_idx, arg);
			queue.push(std::move(entry));
		}

		inline void log(const char* message, u8 caller_idx, u8 type_idx, 
			setting_t arg0, setting_t arg1, setting_t arg2)
		{
			log_entry entry(message, log_format_flags & TIMESTAMP_BIT ? timestamp() : "", caller_idx, type_idx, 
				arg0, arg1, arg2);
			queue.push(std::move(entry));
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
