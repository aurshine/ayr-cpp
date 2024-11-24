#ifndef AYR_BASE_PRINTER_HPP
#define AYR_BASE_PRINTER_HPP

#include <cstdio>
#include <format>
#include <source_location>

#include "CString.hpp"
#include "Object.hpp"
#include "ayr_concepts.hpp"

namespace ayr
{
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
		void __print__(const bool& object) const { std::fprintf(output_file_, object ? "true" : "false"); }

		void __print__(const char& object) const { std::fprintf(output_file_, "%c", object); }

		void __print__(const char* object) const { std::fprintf(output_file_, object); }

		void __print__(std::nullptr_t) const { std::fprintf(output_file_, "nullptr"); }

		void __print__(const std::string& object) const { std::fprintf(output_file_, object.c_str()); }

		void __print__(const CString& object) const { std::fprintf(output_file_, object.data()); }

		template<typename T>
			requires std::is_integral_v<T>
		void __print__(const T& object) const
		{
			if constexpr (std::is_signed_v<T>)
				std::fprintf(output_file_, "%lld", static_cast<long long>(object));
			else
				std::fprintf(output_file_, "%llu", static_cast<unsigned long long>(object));
		}

		template<typename T>
			requires std::is_floating_point_v<T>
		void __print__(const T& object) const
		{
			if constexpr (std::is_same_v<T, float>)
				std::fprintf(output_file_, "%f", object);
			else if constexpr (std::is_same_v<T, double>)
				std::fprintf(output_file_, "%lf", object);
			else if constexpr (std::is_same_v<T, long double>)
				std::fprintf(output_file_, "%Lf", object);
		}

		template<typename T>
			requires std::is_pointer_v<T>
		void __print__(const T object) const { std::fprintf(output_file_, "0x%p", object); }

		template<AyrPrintable T>
		void __print__(const T& object) const { __print__(object.__str__()); }
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