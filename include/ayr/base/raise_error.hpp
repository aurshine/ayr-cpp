#ifndef AYR_BASE_RAISE_ERROR_HPP
#define AYR_BASE_RAISE_ERROR_HPP

#include "printer.hpp"

namespace ayr
{
	// 错误类，抛出后不应该被捕获
	class AyrError
	{
		using self = AyrError;

		CString err_msg_;
	public:
		AyrError(const CString& msg) : err_msg_(msg) {}

		AyrError(CString&& msg) : err_msg_(std::move(msg)) {}

		AyrError(const self& other) : err_msg_(other.err_msg_) {}

		AyrError(self&& other) : err_msg_(std::move(other.err_msg_)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			err_msg_ = other.err_msg_;
			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			err_msg_ = std::move(other.err_msg_);
			return *this;
		}

		CString what() const noexcept { return err_msg_; }
	};

	/*
	* 打印错误信息
	*
	* @param err_name 错误名称
	*
	* @param msg 错误信息
	*/
	template<typename T>
	def pems(const CString& err_name, T&& msg, const ::std::source_location& loc = ::std::source_location::current()) -> CString
	{
		Buffer buffer;
		buffer << "File: " << loc.file_name() << "\n";
		buffer << "Line, Column: " << loc.line() << ", " << loc.column() << "\n";
		buffer << "FunctionName: " << loc.function_name() << "\n";
		buffer << err_name << ": " << msg << "\n";
		ayr_error(vstr(buffer.peek(), buffer.readable_size()));
		return from_buffer(std::move(buffer));
	}

	// 抛出一个错误，并打印错误信息，错误不应该被捕获
#define RAISE(E_NAME, E_MSG) throw ayr::AyrError(pems(E_NAME, E_MSG))

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
#endif // AYR_BASE_RAISE_ERROR_HPP