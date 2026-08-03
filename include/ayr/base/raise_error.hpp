#ifndef AYR_BASE_RAISE_ERROR_HPP
#define AYR_BASE_RAISE_ERROR_HPP

#include "printer.hpp"

namespace ayr
{
	// 错误类，抛出后不应该被捕获
	class AyrError final : public std::exception
	{
		using self = AyrError;

		CString err_name_, err_msg_;

		Buffer err_info_;

		// 在抛异常的时候输出错误信息
		static bool show_error_while_throw_;
	public:
		AyrError(const CString& name, const CString& msg, std::source_location loc = std::source_location::current()) : err_name_(name), err_msg_(msg), err_info_()
		{
			err_info_ << "File: " << loc.file_name() << "\n";
			err_info_ << "Line, Column: " << loc.line() << ", " << loc.column() << "\n";
			err_info_ << "FunctionName: " << loc.function_name() << "\n";
			err_info_ << err_name_ << ": " << err_msg_ << "\n\0";
		}

		AyrError(const self& other) : err_name_(other.err_name_), err_msg_(other.err_msg_), err_info_(other.err_info_) {}

		AyrError(self&& other) noexcept : err_name_(std::move(other.err_name_)), err_msg_(std::move(other.err_msg_)), err_info_(std::move(other.err_info_)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this,std::move(other));
		}
		
		// 适配 c++ exception 接口
		const char* what() const noexcept override { return err_info_.peek(); }

		// 错误所在的文件，行列，函数 + error()
		CString details() const noexcept { return vstr(err_info_.peek(), err_info_.readable_size());  }
		
		// 错误信息 name + message
		CString error() const 
		{ 
			Buffer errbuf(err_name_.size() + err_msg_.size() + 2);
			errbuf << err_name_ << ": " << err_msg_;
			return from_buffer(std::move(errbuf));
		}

		// 错误的名字
		const CString& name() const noexcept { return err_name_; }

		bool is(const CString& name) const { return this->name() == name; }

		// 输出错误信息，抛出异常
		void raise()
		{
			if (show_error_while_throw_)
				ayr_error.write_from_buffer(err_info_);
			throw *this;
		}

		static void show(bool st) { show_error_while_throw_ = st; }
	};

	bool AyrError::show_error_while_throw_ = true;

#define tlog(expr) print(#expr, " = ", expr)

	// 抛出一个错误，并打印错误信息，错误不应该被捕获
#define RAISE(E_NAME, E_MSG) AyrError(ayr::vstr(E_NAME), ayr::vstr(E_MSG)).raise()

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