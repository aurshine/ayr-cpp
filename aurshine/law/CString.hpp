#pragma once
#include <cstring>
#include <string>

#include <law/_ayr.h>


namespace ayr
{
	// c ����ַ�����װ
	class CString: public Ayr
	{
	public:
		CString(const char* str_)
			:str(nullptr)
		{
			size_t len = std::strlen(str_);
			str = new char[len + 1] {};
			memcpy(str, str_, len);
		}

		CString(const CString& other) : CString(other.str) {}

		CString(CString&& other) : str(other.str) { other.str = nullptr; }

		~CString() { delete[] str; }

		CString& operator=(const CString& other)
		{
			if (this == &other)
				return *this;
			
			auto o_str_len = other.size();
			if (size() < o_str_len)
			{
				delete[] str;
				str = new char[o_str_len + 1] {};
			}

			std::memcpy(str, other.str, o_str_len);
			return *this;
		}

		CString& operator=(CString&& other)
		{
			if (this == &other)
				return *this;	
			str = other.str;
			other.str = nullptr;
			return *this;
		}

		size_t size() const { return strlen(str); }

		const char* __str__() const { return str; }

		cmp_t __cmp__(const CString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++ i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];
			return 0;
		}

 		char* str;
	};

	// __str__ ��������ֵ�Ļ�����󳤶�
	constexpr static const size_t __STR_BUFFER_SIZE__ = 128;
	// __str__ ��������ֵ�Ļ���
	static char __str_buffer__[__STR_BUFFER_SIZE__]{};

	inline void memcpy__str_buffer__(const char* str, size_t len)
	{
		size_t cpy_size = std::min(len, __STR_BUFFER_SIZE__ - 1);
		std::memcpy(__str_buffer__, str, cpy_size);
		__str_buffer__[cpy_size] = '\0';
	}

	inline void memcpy__str_buffer__(const std::string& str)
	{
		memcpy__str_buffer__(str.c_str(), str.size());
	}
}