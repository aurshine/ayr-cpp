#pragma once

#ifndef AYR_LAW_DETAIL_CSTRING_HPP
#define AYR_LAW_DETAIL_CSTRING_HPP

#include <cstring>
#include <string>

#include <law/detail/hash.hpp>
#include <law/detail/ayr_memory.hpp>


namespace ayr
{
	// c 风格字符串封装
	template<Char Ch>
	class RawString : public Ayr
	{
	public:
		RawString() : RawString(1) {}

		RawString(const Ch* str_) : RawString(str_, std::strlen(str_ == nullptr ? "" : str_)) {}

		RawString(const std::basic_string<Ch>& str_) : RawString(str_.c_str(), str_.size()) {}

		RawString(c_size len)
		{
			str = ayr_alloc(Ch, len + 1);
			std::memset(str, 0, sizeof(Ch) * (len + 1));
		}

		RawString(const Ch* str_, c_size len_) : RawString(len_)
		{
			for (c_size i = 0; i < len_; ++i)
				str[i] = str_[i];
		}

		RawString(const RawString& other) : RawString(other.str) {}

		RawString(RawString&& other) noexcept : str(other.str) { other.str = nullptr; }

		~RawString() { release(); }

		RawString& operator=(const RawString& other)
		{
			if (this == &other)
				return *this;

			c_size ostr_len = other.size();
			if (size() < ostr_len)
			{
				release();
				str = ayr_alloc(Ch, ostr_len + 1);
			}

			std::memcpy(str, other.str, sizeof(Ch) * (ostr_len + 1));
			return *this;
		}

		RawString& operator=(RawString&& other) noexcept
		{
			if (this == &other)
				return *this;

			release();
			str = other.str;
			other.str = nullptr;

			return *this;
		}

		Ch& operator[] (size_t index) { return str[index]; }

		const Ch& operator[] (size_t index) const { return str[index]; }

		size_t size() const { return std::strlen(str); }

		const Ch* __str__() const { return str; }

		size_t __hash__() const { return bytes_hash(str, std::strlen(str)); }

		void release() { ayr_delloc(str); }

		cmp_t __cmp__(const RawString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];

			return 0;
		}

		Ch* str;
	};

	using CString = RawString<char>;

	inline CString cstr(int64_t value) { return CString(std::to_string(value)); }

	inline CString cstr(uint64_t value) { return CString(std::to_string(value)); }

	inline CString cstr(double value) { return CString(std::to_string(value)); }

	inline CString cstr(bool value) { return  ifelse(value, CString("true"), CString("false")); }

	inline CString cstr(const char* str_) { return CString(str_); }
}
#endif // AYR_LAW_DETAIL_CSTRING_HPP