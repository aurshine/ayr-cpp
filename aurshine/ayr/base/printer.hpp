#ifndef AYR_DETAIL_PRINTER_HPP
#define AYR_DETAIL_PRINTER_HPP

#include <cstdio>
#include <format>
#include <source_location>

#include <ayr/base/CString.hpp>
#include <ayr/base/Object.hpp>

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

		template<typename T>
			requires std::is_pointer_v<T>
		void __print__(const T object) const { std::fprintf(output_file_, "0x%p", object); }

		void __print__(std::nullptr_t) const { std::fprintf(output_file_, "nullptr"); }

		void __print__(const std::string& object) const { std::fprintf(output_file_, object.c_str()); }

		template<AyrPrintable T>
		void __print__(const T& object) const { __print__(object.__str__()); }

		void __print__(const CString& object) const { std::fprintf(output_file_, "%s", object.data()); }
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
				std::format("file: {}\nline, column: {}, {}\nfunction_name: {}\nerror:",
					loc.file_name(),
					loc.line(),
					loc.column(),
					loc.function_name()),
				msg
			);
		}
	}

	template<Printable T>
	inline void error_exec(const T& msg, const ::std::source_location& loc = ::std::source_location::current())
	{
		ayr_error(
			std::format("file: {}\nline, column: {}, {}\nfunction_name: {}\n",
				loc.file_name(),
				loc.line(),
				loc.column(),
				loc.function_name()),
			msg
		);

		throw std::runtime_error("");
	}


#define Error(errorname, msg) error_exec(std::format("{}: {}", errorname, msg))

#define KeyError(msg) Error("KeyError", msg)

#define ValueError(msg) Error("ValueError", msg)

#define TypeError(msg) Error("TypeError", msg)

#define RuntimeError(msg) Error("RuntimeError", msg)

#define NotImplementedError(msg) Error("NotImplementedError", msg)

#define FileNotFoundError(msg) Error("FileNotFoundError", msg)

#define PermissionError(msg) Error("PermissionError", msg)

#define EncodingError(msg) Error("EncodingError", msg)

#define SystemError(msg) Error("SystemError", msg)
}

template<ayr::AyrObject Ayr>
struct std::formatter<Ayr> : std::formatter<const char*>
{
	auto format(const Ayr& value, std::format_context& ctx) const
	{
		return std::formatter<const char*>::format(value.__str__().data(), ctx);
	}
};

#endif // AYR_DETAIL_PRINTER_HPP