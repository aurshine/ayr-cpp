#ifndef AYR_BASE_PRINTER_HPP
#define AYR_BASE_PRINTER_HPP

#include <cstdio>
#include <format>
#include <source_location>

#include "Object.hpp"

namespace ayr
{
	class Printer : public Object<Printer>
	{
	public:
		Printer(FILE* file_ptr, CString sw = " ", CString ew = "\n") : output_file_(file_ptr), sw_(std::move(sw)), ew_(std::move(ew)) {}

		template<typename T0, typename... Args>
		void operator()(const T0& object, const Args&... args) const
		{
			Buffer buffer(128);
			write_buffer(buffer, object, args...);
			std::fwrite(buffer.data(), 1, buffer.size(), output_file_);
		}

		void operator()() const
		{
			Buffer buffer(128);
			write_buffer(buffer);
			std::fwrite(buffer.data(), 1, buffer.size(), output_file_);
		}

		void flush() const { std::fflush(output_file_); }

		// 设置输出结束符
		void setend(CString ew) { ew_ = std::move(ew); }

		// 设置输出分隔符
		void setsep(CString sw) { sw_ = std::move(sw); }
	protected:
		template<typename First, typename... Args>
		void write_buffer(Buffer& buffer, First&& first, Args&&... args) const
		{
			buffer << first;
			if constexpr (sizeof...(args) > 0)
				((buffer << sw_ << args), ...);
			buffer << ew_;
		}

		void write_buffer(Buffer& buffer) const { buffer << ew_; }

		CString ew_; // 结束符

		CString sw_; // 分隔符

		std::FILE* output_file_;
	};


	class Color : public Object<Color>
	{
	public:
		constexpr static const char* CLOSE = "\033[0m";

		constexpr static const char* BLACK = "\033[30m";

		constexpr static const char* RED = "\033[31m";

		constexpr static const char* GREEN = "\033[32m";

		constexpr static const char* YELLOW = "\033[33m";

		constexpr static const char* BLUE = "\033[34m";

		constexpr static const char* PURPLE = "\033[35m";

		constexpr static const char* DEEPGREEN = "\033[36m";

		constexpr static const char* WHITE = "\033[37m";

		constexpr static const char* AQUA = "\033[94m";
	};

	class ColorPrinter : public Printer
	{
		using super = Printer;
	public:
		ColorPrinter(FILE* file_ptr, CString color = Color::WHITE)
			: Printer(file_ptr), color_(std::move(color)) {}

		template<typename... Args>
		void operator()(const Args&... args) const
		{
			Buffer buffer(128);
			buffer << color_;
			super::write_buffer(buffer, args...);
			buffer << Color::CLOSE;
			std::fwrite(buffer.data(), 1, buffer.size(), output_file_);
		}

		void setcolor(CString color) { color_ = std::move(color); }
	private:
		CString color_;
	};

	static Printer print{ stdout };

	static ColorPrinter ayr_warner{ stdout, Color::YELLOW };

	static ColorPrinter ayr_error{ stderr, Color::RED };
}

template<ayr::AyrObject Ayr>
struct std::formatter<Ayr> : std::formatter<ayr::CString>
{
	auto format(const Ayr& value, std::format_context& ctx) const
	{
		ayr::CString str = ayr::cstr(value);
		return std::formatter<ayr::CString>::format(str, ctx);
	}
};

#define tlog(expr) print(#expr, " = ", expr)

#endif // AYR_BASE_PRINTER_HPP