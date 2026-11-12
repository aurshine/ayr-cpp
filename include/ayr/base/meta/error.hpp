#ifndef AYR_BASE_META_ERROR_HPP
#define AYR_BASE_META_ERROR_HPP

#include <source_location>

#include "fmt.hpp"

namespace ayr
{
	// 错误类，抛出后不应该被捕获
	class AyrError
	{
		using self = AyrError;

		CString err_name_, err_msg_, err_info_;
	public:
		// 在抛异常的时候输出错误信息
		static bool show_error_while_throw;

		constexpr AyrError(const CString& name, const CString& msg, std::source_location loc = std::source_location::current()) : err_name_(name), err_msg_(msg), err_info_()
		{
			err_info_ = fmt("File: {}\nLine，Column: {}, {}\nFunctionName: {}\n{}: {}\n\0",
				loc.file_name(),
				loc.line(),
				loc.column(),
				loc.function_name(),
				err_name_,
				err_msg_
			);
		}

		constexpr AyrError(const self& other) : err_name_(other.err_name_), err_msg_(other.err_msg_), err_info_(other.err_info_) {}

		constexpr AyrError(self&& other) noexcept : err_name_(std::move(other.err_name_)), err_msg_(std::move(other.err_msg_)), err_info_(std::move(other.err_info_)) {}

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
			return *ayr_construct(this, std::move(other));
		}

		// 适配 c++ exception 接口
		constexpr const char* what() const noexcept { return err_info_.data(); }

		// 错误所在的文件，行列，函数 + error()
		constexpr const CString& details() const noexcept { return err_info_; }

		// 错误信息 name + message
		constexpr CString error() const { return fmt("{}: {}", err_name_, err_msg_); }

		// 错误的名字
		constexpr const CString& name() const noexcept { return err_name_; }

		constexpr bool is(const CString& name) const { return this->name() == name; }

		// 设置抛出异常时是否输出错误信息
		static void show(bool st) { show_error_while_throw = st; }
	};

	bool AyrError::show_error_while_throw = true;
}
#endif // AYR_BASE_META_ERROR_HPP