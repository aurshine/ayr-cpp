#pragma once
#include <cstring>
#include <string>

#include <law/object.hpp>


namespace ayr
{
	inline constexpr uint32_t decode_fixed32(const char* ptr)
	{
		const uint8_t* buffer = reinterpret_cast<const uint8_t*>(ptr);

		return (
			static_cast<uint32_t>(buffer[0]) |
			(static_cast<uint32_t>(buffer[1]) << 8) |
			(static_cast<uint32_t>(buffer[2]) << 16) |
			(static_cast<uint32_t>(buffer[3]) << 24)
			);
	}


	inline constexpr hash_t bytes_hash(const char* data, size_t n, uint32_t seed = 0xbc9f1d34)
	{
		constexpr hash_t m = 0xc6a4a793;
		constexpr hash_t r = 24;
		const char* end = data + n;
		hash_t h = seed ^ (n * m);

		while (data < end)
		{
			hash_t w = decode_fixed32(data);
			data += 4;
			h = (h + w) * m;
			h ^= (h >> 16);
		}

		int dis = end - data;
		while (dis--)
		{
			h += static_cast<uint8_t>(data[dis - 1] << (dis - 1) * 8);
		}
		h *= m;
		h ^= (h >> r);
		return h;
	}


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

		size_t __hash__() const { return bytes_hash(str, std::strlen(str), 0); }

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