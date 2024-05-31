#pragma once
#include <cstring>
#include <string>

#include <law/_ayr.h>



template<ayr::Char T>
struct std::hash<T*>
{
	size_t operator()(const T* c_str) const noexcept
	{
		static size_t P = 1331;
		size_t hash_value = 0;
		for (size_t i = 0; c_str[i] != '\0'; i++)
			hash_value = hash_value * P + c_str[i];

		return hash_value;
	}
};

namespace ayr
{
	


	// c 风格字符串封装
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

		char& operator[] (size_t index) { return str[index]; }

		const char& operator[] (size_t index) const { return str[index]; }

		size_t size() const { return strlen(str); }

		const char* __str__() const { return str; }
		
		size_t __hash__() const { return std::hash<char*>()(str); }

		cmp_t __cmp__(const CString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++ i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];
			return 0;
		}

 		char* str;
	};

	// __str__ 方法返回值的缓存最大长度
	constexpr static const size_t __STR_BUFFER_SIZE__ = 128;
	// __str__ 方法返回值的缓存
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