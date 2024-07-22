#pragma once
#include <iostream>
#include <format>
#include <source_location>

#include <law/detail/object.hpp>
#include <law/detail/CString.hpp>

namespace ayr
{
	template<class Ostream>
	class Printer : public Object
	{
	public:
		Printer(Ostream& ostream) : Printer("\n", " ", ostream) {}

		Printer(CString ew, CString sw, Ostream& ostream)
			: ew_(std::move(ew)), sw_(std::move(sw)), ostream(ostream) {}


		template<Printable T>
		void operator()(const T& object) const { __print__(object); __print__(ew_); }

		template<Printable T, Printable... Args>
		void operator()(const T& object, const Args&... args) const { __print__(object, args...); __print__(ew_); }


		// 设置输出结束符
		void set_end_word(CString ew) const { ew_ = std::move(ew); }

		// 设置输出分隔符
		void set_sep_word(CString sw) const { sw_ = std::move(sw); }

	protected:
		// 单一形参
		template<Printable T>
		void __print__(const T& object) const { ostream << object; }

		void __print__(const bool& object) const { ostream << (object ? "true" : "false"); }

		void __print__(const CString& object) const { ostream << object.str; }

		// 可变形参
		template<Printable T, Printable ...Args>
		void __print__(const T& object, const Args& ...args) const
		{
			__print__(object);
			__print__(sw_);
			__print__(args...);
		}

	private:
		CString ew_; // 结束符

		CString sw_; // 分隔符

		Ostream& ostream;
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
	};


	template<class Ostream>
	class ColorPrinter : public Printer<Ostream>
	{
		using super = Printer<Ostream>;
	public:
		ColorPrinter(Ostream& ostream, const char* color = Color::WHITE)
			: Printer<Ostream>(ostream), color_(color) {}

		~ColorPrinter() = default;

		template<Printable T>
		void operator()(const T& object) const
		{
			opencolor();
			super::operator()(object);
			closecolor();
		}

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
		const char* color_;
	};


	static Printer<std::ostream> print{ std::cout };

	static ColorPrinter<std::ostream> ayr_warner{ std::cout, Color::YELLOW };

	static ColorPrinter<std::ostream> ayr_error{ std::cout, Color::RED };


	template<Printable T>
	inline void warn_assert(bool condition, const T& msg, const std::source_location& loc = std::source_location::current())
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
	inline void error_assert(bool condition, const T& msg, const std::source_location& loc = std::source_location::current())
	{
		if (!condition)
		{
			ayr_error(
				std::format("file: {}  column: {}  line: {}  function_name: {}\n"\
					"error: ",
					loc.file_name(),
					loc.column(),
					loc.line(),
					loc.function_name()),
				msg
			);
			exit(-1);
		}
	}


#define KeyError(msg) error_assert(false, std::format("KeyError: {}", msg))

#define ValueError(msg) error_assert(false, std::format("ValueError: {}", msg))

#define TypeError(msg) error_assert(false, std::format("TypeError: {}", msg))

#define RuntimeError(msg) error_assert(false, std::format("RuntimeError: {}", msg))

#define NotImplementedError(msg) error_assert(false, std::format("NotImplementedError: {}", msg))
}
