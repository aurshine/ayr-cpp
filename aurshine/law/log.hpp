#ifndef AYR_LAW_LOG_HPP
#define AYR_LAW_LOG_HPP

#include <ctime>
#include <functional>

#include <law/printer.hpp>
#include <law/timer.hpp>
#include <law/Array.hpp>


namespace ayr
{
	class Log : Object
	{
	public:
		struct LogLevel
		{
			constexpr static int TRACE = 0;

			constexpr static int DEBUG = 1;

			constexpr static int INFO = 2;

			constexpr static int WARN = 3;

			constexpr static int ERROR = 4;

			constexpr static int FATAL = 5;

			constexpr static int INACTIVE = INT_MAX;

			constexpr static const char* LEVEL_NAMES[] = {
				"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
			};

			constexpr static const char* LEVEL_COLORS[] = {
				Color::AQUA, Color::DEEPGREEN, Color::GREEN, Color::YELLOW, Color::RED, Color::PURPLE
			};
		};


		class LogEvent : Object
		{
		public:
			constexpr LogEvent() : level_(LogLevel::INACTIVE), output_(nullptr) {}

			constexpr LogEvent(int level, FILE* output) : level_(level), output_(output) {}

			int level_;

			FILE* output_;
		};

		static void print_logevent(const LogEvent& evt, const char* msg, Date date, const char* file, int line)
		{
			fprintf(evt.output_, "%s %s%-5s%s %s:%d", date.__str__().str, LogLevel::LEVEL_COLORS[evt.level_], LogLevel::LEVEL_NAMES[evt.level_], Color::CLOSE, file, line);
			fprintf(evt.output_, msg);
			fprintf(evt.output_, "\n");
			fflush(evt.output_);
		}


		static void add_log(const LogEvent& evt)
		{
			if (event_count + 1 >= MAX_LOG_SIZE)
				RuntimeError("Log event buffer overflow");

			events[event_count++] = evt;
		}

		static void log(const char* msg, int level, const char* file, int line)
		{
			time_t cur_t = std::time(nullptr);
			tm* t = std::localtime(&cur_t);

			char log_tm[16];
			log_tm[strftime(log_tm, sizeof(log_tm), "%H:%M:%S", t)] = '\0';

			for (int i = 0; i < event_count; ++i)
				if (events[i].level_ <= level)
					print_logevent(events[i], msg, file, log_tm, line);
		}

	private:
		constexpr static int MAX_LOG_SIZE = 64;

		static LogEvent events[MAX_LOG_SIZE];

		static int event_count;
	};
	
int Log::event_count = 0;


#define log_trace(msg) ayr::Log::log(msg, LogLevel::TRACE, __FILE__, __LINE__)

#define log_debug(msg) ayr::Log::log(msg, LogLevel::DEBUG, __FILE__, __LINE__)

#define log_info(msg) ayr::Log::log(msg, LogLevel::INFO, __FILE__, __LINE__)

#define log_warn(msg) ayr::Log::log(msg, LogLevel::WARN, __FILE__, __LINE__)

#define log_error(msg) ayr::Log::log(msg, LogLevel::ERROR, __FILE__, __LINE__)

#define log_fatal(msg) ayr::Log::log(msg, LogLevel::FATAL, __FILE__, __LINE__)
}
#endif