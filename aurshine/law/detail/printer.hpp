#pragma once

#include <cstdio>
#include <format>
#include <source_location>

#include <law/detail/object.hpp>

namespace ayr
{
	class Printer : public Object
	{
	public:
		Printer(FILE* file_ptr, CString sw=" ", CString ew="\n") : output_file_(file_ptr), sw_(std::move(sw)), ew_(std::move(ew)) {}

		template<Printable... Args>
		void operator()(const Args&... args) const { __print__(args...); __print__(ew_); }

		// 设置输出结束符
		void set_end(CString ew) { ew_ = std::move(ew); }

		// 设置输出分隔符
		void set_sep(CString sw) { sw_ = std::move(sw); }

	protected:
		// 单一形参
		void __print__() const {}

		void __print__(const bool& object) const { std::fprintf(output_file_, object ? "true" : "false"); }

		void __print__(const int& object) const { std::fprintf(output_file_, "%d", object); }

		void __print__(const unsigned int& object) const { std::fprintf(output_file_, "%u", object); }

		void __print__(const long& object) const { std::fprintf(output_file_, "%ld", object); }

		void __print__(const long long& object) const { std::fprintf(output_file_, "%lld", object); }

		void __print__(const unsigned long& object) const { std::fprintf(output_file_, "%lu", object); }

		void __print__(const unsigned long long& object) const { std::fprintf(output_file_, "%llu", object); }

		void __print__(const float& object) const { std::fprintf(output_file_, "%f", object); }

		void __print__(const double& object) const { std::fprintf(output_file_, "%lf", object); }

		void __print__(const long double& object) const { std::fprintf(output_file_, "%Lf", object); }

		void __print__(const char& object) const { std::fprintf(output_file_, "%c", object); }

		void __print__(const char* object) const { std::fprintf(output_file_, object); }

		void __print_ptr__(const void* object) const { std::fprintf(output_file_, "0x%p", object); }

		void __print__(const std::nullptr_t& object) const { std::fprintf(output_file_, "nullptr"); }

		void __print__(const std::string& object) const { std::fprintf(output_file_, object.c_str()); }

		template<AyrPrintable T>
		void __print__(const T& object) const { __print__(object.__str__()); }

		void __print__(const CString& object) const { std::fprintf(output_file_, "%s", object.str); }

		// 可变形参
		template<Printable T, Printable ...Args>
		void __print__(const T& object, const Args& ...args) const
		{
			if constexpr (std::is_pointer_v<T>)
				__print_ptr__(object);
			else
				__print__(object);

			__print__(sw_);
			__print__(args...);
		}
	private:
		CString ew_; // 结束符

		CString sw_; // 分隔符

		std::FILE* output_file_;
	};


	class Color : Ayr
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
		ColorPrinter(FILE* file_ptr, const char* color = Color::WHITE)
			: Printer(file_ptr), color_(color) {}

		template<Printable T, Printable... Args>
		void operator()(const T& object, const Args&... args) const
		{
			opencolor();
			super::operator()(object, args...);
			closecolor();
		}

		void opencolor() const { super::__print__(color_); }

		void closecolor() const { super::__print__(Color::CLOSE); }

	private:
		CString color_;
	};


	static Printer print{ stdout };

	static ColorPrinter ayr_warner{ stdin, Color::YELLOW };

	static ColorPrinter ayr_error{ stderr, Color::RED };


	template<Printable T>
	inline void warn_assert(bool condition, const T& msg, const ::std::source_location& loc = ::std::source_location::current())
	{
		if (!condition)
		{
			ayr_warner(
				std::format("file: {}  column: {} line: {} function_name: {} \n"\
					"error: ",
					loc.file_name(),
					loc.column(),
					loc.line(),
					loc.function_name()),
				msg
			);
		}
	}

	template<Printable T>
	inline void error_assert(bool condition, const T& msg, const ::std::source_location& loc = ::std::source_location::current())
	{
		if (!condition)
		{
			ayr_error(
				std::format("file: {}\nline: {}\ncolumn: {}\nfunction_name: {}\nerror: ",
					loc.file_name(),
					loc.line(),
					loc.column(),
					loc.function_name()),
				msg
			);
			exit(-1);
		}
	}


#define Error(errorname, msg) error_assert(false, std::format("{}: {}", errorname, msg))

#define KeyError(msg) Error("KeyError", msg)

#define ValueError(msg) Error("ValueError", msg)

#define TypeError(msg) Error("TypeError", msg)

#define RuntimeError(msg) Error("RuntimeError", msg)

#define NotImplementedError(msg) Error("NotImplementedError", msg)

#define FileNotFoundError(msg) Error("FileNotFoundError", msg)

#define PermissionError(msg) Error("PermissionError", msg)
}