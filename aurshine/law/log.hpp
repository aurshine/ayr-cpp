#ifndef AYR_LAW_LOG_HPP
#define AYR_LAW_LOG_HPP

#include <ctime>
#include <functional>

#include <law/printer.hpp>

namespace ayr
{
	typedef void(*LogFn)(LogEvent& event);

	typedef void(*LogLockFn)(bool lock, void* data);

	constexpr int MAX_CALLBACKS = 32;

	struct LogEvent
	{
		LogEvent(const char* msg, const char* file, FILE* output, int line, int level)
			: msg(msg), file(file), output(output), line(line), level(level)
		{
			time_t _t = time(nullptr);
			t = localtime(&_t);
		}

		const char* msg;

		const char* file;

		tm* t;

		FILE* output;

		int line;

		int level;
	};

	struct LogLevel
	{
		constexpr static int TRACE = 0;

		constexpr static int DEBUG = 1;

		constexpr static int INFO = 2;

		constexpr static int WARN = 3;

		constexpr static int ERROR = 4;

		constexpr static int FATAL = 5;

		constexpr static const char* LEVEL_NAMES[] = {
			"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
		};

		constexpr static const char* LEVEL_COLORS[] = {
			Color::AQUA, Color::DEEPGREEN, Color::GREEN, Color::YELLOW, Color::RED, Color::PURPLE
		};
	};

	struct LogCallback
	{
		LogFn fn;
		FILE* output;
		int level;
	};

	struct L
	{
		FILE* output;
		LogLockFn lock_fn;
		int level;
		bool quiet;
		LogCallback callbacks[MAX_CALLBACKS];

		L() = default;

		L& operator=(const L&) = delete;

		L& operator=(L&&) = delete;

		L(const L&) = delete;

		L(L&&) = delete;

		static L& instance()
		{
			static L inst;
			return inst;
		}
	};


	def stdout_callback(LogEvent& event)
	{
		char buffer[16];
		buffer[strftime(buffer, sizeof(buffer), "%H:%M:%S", event.t)] = '\0';
		fprintf(event.output, "%s %s%-5s%s %s:%d %s ", buffer, LogLevel::LEVEL_COLORS[event.level], LogLevel::LEVEL_NAMES[event.level], Color::CLOSE, event.file, event.line);
		fprintf(event.output, event.msg);
		fprintf(event.output, "\n");
		fflush(event.output);
	}

	def set_level(int level)
	{
		L::instance().level = level;
	}

	def set_quiet(bool quiet)
	{
		L::instance().quiet = quiet;
	}

	def add_callback(LogFn fn, FILE* output, int level)
	{
		for (int i = 0; i < MAX_CALLBACKS; i++)
			if (L::instance().callbacks[i].fn == nullptr)
			{
				L::instance().callbacks[i] = LogCallback{ fn, output, level };
				break;
			}
	}

	def add_fp(FILE* fp, int level)
	{
		return add_callback(stdout_callback, fp, level);
	}


	void log(const char* msg, int level, const char* file, int line)
	{
		LogEvent log_event{ msg, file, stderr, line, level };

	}

#define log_trace(fmt, ...) log(LogLevel::TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define log_debug(fmt, ...) log(LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define log_info(fmt, ...) log(LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define log_warn(fmt, ...) log(LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define log_error(fmt, ...) log(LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define log_fatal(fmt, ...) log(LogLevel::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
}
#endif