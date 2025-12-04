#ifndef AYR_AIR_LOG_HPP
#define AYR_AIR_LOG_HPP

#include <functional>

#include "../base.hpp"

namespace ayr
{
	class Log
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

			constexpr static int ERR = 4;

			constexpr static int FATAL = 5;

			constexpr static int INACTIVE = INT_MAX;

			constexpr static const char* LEVEL_NAMES[] = {
				"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
			};

			constexpr static const char* LEVEL_COLORS[] = {
				Color::AQUA, Color::DEEPGREEN, Color::GREEN, Color::YELLOW, Color::RED, Color::PURPLE
			};
		};


		class LogEvent
		{
		public:
			constexpr LogEvent() : level_(LogLevel::INACTIVE), output_(nullptr) {}

			constexpr LogEvent(int level, FILE* output) : level_(level), output_(output) {}

			constexpr LogEvent(const LogEvent& other) : level_(other.level_), output_(other.output_) {}

			~LogEvent()
			{
				if (output_ && output_ != stdout && output_ != stderr)
					fclose(output_);
			}

			constexpr LogEvent& operator=(const LogEvent& other)
			{
				level_ = other.level_;
				output_ = other.output_;
				return *this;
			}

			int level_;

			FILE* output_;
		};

		static void print_logevent(const LogEvent& evt, const CString& msg, const Date& date, const CString& file, int line)
		{
			fprintf(evt.output_, "%s %s%-5s%s %s:%d ",
				cstr(date).c_str(),
				LogLevel::LEVEL_COLORS[evt.level_],
				LogLevel::LEVEL_NAMES[evt.level_],
				Color::CLOSE,
				file.c_str(),
				line);
			fwrite(msg.data(), 1, msg.size(), evt.output_);
			fprintf(evt.output_, "\n");
			fflush(evt.output_);
		}

		static void add_log(int level, FILE* output)
		{
			if (event_count + 1 >= MAX_LOG_SIZE)
				RuntimeError("Log event buffer overflow");

			events[event_count++] = LogEvent(level, output);
		}

		static void add_log(int level, const char* filename) { add_log(level, std::fopen(filename, "w")); }

		static void log(const CString& msg, int level, const CString& file, int line, Date date = Date{})
		{
			for (int i = 0; i < event_count; ++i)
				if (events[i].level_ <= level)
					print_logevent(events[i], msg, date, file, line);
		}

		static void trace(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::TRACE, loc.file_name(), loc.line()); }

		static void debug(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::DEBUG, loc.file_name(), loc.line()); }

		static void info(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::INFO, loc.file_name(), loc.line()); }

		static void warn(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::WARN, loc.file_name(), loc.line()); }

		static void error(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::ERR, loc.file_name(), loc.line()); }

		static void fatal(const CString& msg, std::source_location loc = std::source_location::current()) { return log(msg, LogLevel::FATAL, loc.file_name(), loc.line()); }

	private:
		constexpr static int MAX_LOG_SIZE = 64;

		static LogEvent events[MAX_LOG_SIZE];

		static int event_count;
	};

	Log::LogEvent Log::events[MAX_LOG_SIZE] = { Log::LogEvent(Log::LogLevel::TRACE, stdout) };

	int Log::event_count = 1;
}
#endif // AYR_AIR_LOG_HPP