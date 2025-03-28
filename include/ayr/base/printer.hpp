﻿#ifndef AYR_BASE_PRINTER_HPP
#define AYR_BASE_PRINTER_HPP

#include <cstdio>
#include <format>
#include <source_location>

#include "CString.hpp"
#include "Object.hpp"
#include "ayr_concepts.hpp"

namespace ayr
{
	struct _Flush : public Object<_Flush> {};

	constexpr static const _Flush flush{};

	der(std::ostream&) operator<<(std::ostream& os, const _Flush& flush)
	{
		os.flush();
		return os;
	}

	class Printer : public Object<Printer>
	{
	public:
		Printer(FILE* file_ptr, CString sw = " ", CString ew = "\n") : output_file_(file_ptr), sw_(std::move(sw)), ew_(std::move(ew)) {}

		template<Printable T0, Printable... Args>
		void operator()(const T0& object, const Args&... args) const
		{
			__print__(object);
			if constexpr (sizeof...(args) > 0)
				((__print__(sw_), __print__(args)), ...);
			__print__(ew_);
		}

		void operator()() const { __print__(ew_); }

		// 设置输出结束符
		void setend(CString ew) { ew_ = std::move(ew); }

		// 设置输出分隔符
		void setsep(CString sw) { sw_ = std::move(sw); }

	protected:
		void __print__(const _Flush& flush) const { std::fflush(output_file_); }

		template<Printable P>
		void __print__(const P& object) const { std::fprintf(output_file_, cstr(object)); }
	private:
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

		template<Printable T, Printable... Args>
		void operator()(const T& object, const Args&... args) const
		{
			opencolor();
			super::operator()(object, args...);
			closecolor();
		}

		void opencolor() const { super::__print__(color_); }

		void closecolor() const { super::__print__(Color::CLOSE); }

		void setcolor(CString color) { color_ = std::move(color); }
	private:
		CString color_;
	};


	static Printer print{ stdout };

	static ColorPrinter ayr_warner{ stdin, Color::YELLOW };

	static ColorPrinter ayr_error{ stderr, Color::RED };
}

template<ayr::AyrPrintable Ayr>
struct std::formatter<Ayr> : std::formatter<const char*>
{
	auto format(const Ayr& value, std::format_context& ctx) const
	{
		return std::formatter<const char*>::format(static_cast<const char*>(value.__str__()), ctx);
	}
};

#define tlog(expr) print(#expr, " = ", expr)

#endif // AYR_BASE_PRINTER_HPP