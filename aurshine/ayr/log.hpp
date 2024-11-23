#ifndef AYR_LOG_HPP
#define AYR_LOG_HPP

#include <functional>

#include "timer.hpp"
#include "Array.hpp"

namespace ayr
{
	class Log : Object<Log>
	{
	private:
		Log() = delete;

		Log(const Log&) = delete;

		Log& operator=(const Log&) = delete;
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

		static void print_logevent(const LogEvent& evt, const char* msg, const Date& date, const char* file, int line)
		{
			fprintf(evt.output_, "%s %s%-5s%s %s:%d ", date.__str__(), LogLevel::LEVEL_COLORS[evt.level_], LogLevel::LEVEL_NAMES[evt.level_], Color::CLOSE, file, line);
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

		template<ConveribleToCstr Str>
		static void log(const Str msg, int level, const Str file, int line, Date date = Date{})
		{
			for (int i = 0; i < event_count; ++i)
				if (events[i].level_ <= level)
					print_logevent(events[i], msg, date, file, line);
		}

		template<ConveribleToCstr Str>
		static void trace(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::TRACE, loc.file_name(), loc.line()); }

		template<ConveribleToCstr Str>
		static void debug(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::DEBUG, loc.file_name(), loc.line()); }

		template<ConveribleToCstr Str>
		static void info(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::INFO, loc.file_name(), loc.line()); }

		template<ConveribleToCstr Str>
		static void warn(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::WARN, loc.file_name(), loc.line()); }

		template<ConveribleToCstr Str>
		static void error(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::ERROR, loc.file_name(), loc.line()); }

		template<ConveribleToCstr Str>
		static void fatal(const Str& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::FATAL, loc.file_name(), loc.line()); }

	private:
		constexpr static int MAX_LOG_SIZE = 64;

		static LogEvent events[MAX_LOG_SIZE];

		static int event_count;
	};

	Log::LogEvent Log::events[MAX_LOG_SIZE] = { Log::LogEvent(Log::LogLevel::TRACE, stdout) };

	int Log::event_count = 1;
}
#endif