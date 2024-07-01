#pragma once
#include <cstring>
#include <string>

#include <law/object.hpp>


namespace ayr
{
	template<typename T>
	size_t char_hash(const T& c_str) noexcept
	{
		static size_t P = 1331;
		size_t hash_value = 0;
		for (size_t i = 0; c_str[i] != '\0'; i++)
			hash_value = hash_value * P + c_str[i];

		return hash_value;
	};


	// c ·ç¸ñ×Ö·û´®·â×°
	class CString: public Ayr
	{
	public:
		CString(): str(nullptr) {}

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
		
		size_t __hash__() const { return char_hash(str); }

		cmp_t __cmp__(const CString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++ i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];
			return 0;
		}

 		char* str;
	};
}