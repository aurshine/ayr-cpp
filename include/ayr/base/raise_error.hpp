#ifndef AYR_BASE_RAISE_ERROR_HPP_
#define AYR_BASE_RAISE_ERROR_HPP_

#include "printer.hpp"

namespace ayr
{
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

#define NullPointerError(msg) Error("NullPointerError", msg)

#define FileNotFoundError(msg) Error("FileNotFoundError", msg)

#define PermissionError(msg) Error("PermissionError", msg)

#define EncodingError(msg) Error("EncodingError", msg)

#define SystemError(msg) Error("SystemError", msg)
}
#endif // AYR_BASE_RAISE_ERROR_HPP_