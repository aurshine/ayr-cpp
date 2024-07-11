#pragma once
#include <cstring>
#include <string>

#include <law/object.hpp>
#include <law/hash.hpp>


namespace ayr
{
	// c 风格字符串封装
	class CString : public Ayr
	{
	public:
		CString() : str(nullptr) {}

		CString(const char* str_)
			:str(nullptr)
		{
			size_t len = std::strlen(str_);
			str = new char[len + 1] {};
			strncpy(str, str_, len + 1);
		}

		CString(const CString& other) : CString(other.str) {}

		CString(CString&& other) noexcept : str(other.str) { other.str = nullptr; }

		~CString() { delete[] str; }

		CString& operator=(const CString& other)
		{
			if (this == &other)
				return *this;

			size_t o_str_len = other.size();
			if (size() < o_str_len)
			{
				delete[] str;
				str = new char[o_str_len + 1] {};
			}

			strncpy(str, other.str, o_str_len + 1);
			return *this;
		}

		CString& operator=(CString&& other) noexcept
		{
			if (this == &other)
				return *this;

			str = other.str;
			other.str = nullptr;

			return *this;
		}

		char& operator[] (size_t index) { return str[index]; }

		const char& operator[] (size_t index) const { return str[index]; }

		size_t size() const { return strlen(str); }

		const char* __str__() const { return str; }

		size_t __hash__() const { return bytes_hash(str, std::strlen(str)); }

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