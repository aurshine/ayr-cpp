#pragma once
#include <cstring>

#include <law/detail/hash.hpp>
#include <law/detail/ayr_memory.hpp>


namespace ayr
{
	// c 风格字符串封装
	class CString : public Ayr
	{
	public:
		CString() : str(nullptr) {}

		CString(const char* str_) : CString(str_, std::strlen(str_)) {}

		CString(const char* str_, size_t len_)
		{
			str = ayr_alloc(char, len_ + 1);
			std::memcpy(str, str_, len_);
			str[len_] = '\0';
		}

		CString(const CString& other) : CString(other.str) {}

		CString(CString&& other) noexcept : str(other.str) { other.str = nullptr; }

		~CString() { release(); }

		CString& operator=(const CString& other)
		{
			if (this == &other)
				return *this;

			size_t ostr_len = other.size();
			if (size() < ostr_len)
			{
				release();
				str = new char[ostr_len + 1] {};
			}

			memcpy(str, other.str, ostr_len + 1);
			return *this;
		}

		CString& operator=(CString&& other) noexcept
		{
			if (this == &other)
				return *this;

			release();
			str = other.str;
			other.str = nullptr;

			return *this;
		}

		char& operator[] (size_t index) { return str[index]; }

		const char& operator[] (size_t index) const { return str[index]; }

		size_t size() const { return strlen(str); }

		const char* __str__() const { return str; }

		size_t __hash__() const { return bytes_hash(str, std::strlen(str)); }

		void release() { delete[] str; str = nullptr; }

		cmp_t __cmp__(const CString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];
			return 0;
		}

		char* str;
	};
}