#ifndef AYR_BASE_META_HPP
#define AYR_BASE_META_HPP

#include "meta/ayr_concepts.hpp"
#include "meta/Buffer.hpp"
#include "meta/CString.hpp"
#include "meta/error.hpp"
#include "meta/fmt.hpp"
#include "meta/fmt_wrapper.hpp"
#include "meta/hash.hpp"
#include "meta/None.hpp"
#include "meta/Printer.hpp"

namespace ayr
{
	// 抛出异常并打印错误信息
	def raise_error(const AyrError& e)
	{
		if (AyrError::show_error_while_throw)
			ayr_error(e.details());
		throw e;
	}

#define tlog(expr) print(#expr, " = ", expr)

	// 抛出一个错误，并打印错误信息，错误不应该被捕获
#define RAISE(E_NAME, E_MSG) raise_error(AyrError(E_NAME, E_MSG))

#define KeyError(E_MSG) RAISE("KeyError", E_MSG)

#define ValueError(E_MSG) RAISE("ValueError", E_MSG)

#define TypeError(E_MSG) RAISE("TypeError", E_MSG)

#define RuntimeError(E_MSG) RAISE("RuntimeError", E_MSG)

#define NotImplementedError(E_MSG) RAISE("NotImplementedError", E_MSG)

#define NullPointerError(E_MSG) RAISE("NullPointerError", E_MSG)

#define FileNotFoundError(E_MSG) RAISE("FileNotFoundError", E_MSG)

#define PermissionError(E_MSG) RAISE("PermissionError", E_MSG)

#define EncodingError(E_MSG) RAISE("EncodingError", E_MSG)

#define SystemError(E_MSG) RAISE("SystemError", E_MSG)

#define SSLError(E_MSG) RAISE("SSLError", E_MSG)

#define JsonValueError(E_MSG) RAISE("JsonValueError", E_MSG)
}
#endif // AYR_BASE_META_HPP
