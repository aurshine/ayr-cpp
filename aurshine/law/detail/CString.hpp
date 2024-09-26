#ifndef AYR_LAW_DETAIL_CSTRING_HPP
#define AYR_LAW_DETAIL_CSTRING_HPP

#include <cstring>
#include <string>

#include <law/detail/hash.hpp>
#include <law/detail/ayr_memory.hpp>


namespace ayr
{
	class CString
	{
	public:
		CString() : str(std::make_unique<char[]>(1)) {}

		CString(c_size len) : str(std::make_unique<char[]>(len + 1)) {}

		CString(const char* str_) : CString(str_, std::strlen(str_)) {}

		CString(const char* str_, c_size len_) : CString(len_) { std::memcpy(data(), str_, sizeof(char) * len_); }

		CString(const std::basic_string<char>& str_) : CString(str_.c_str(), str_.size()) {}

		CString(const CString& other) : CString(other.data(), other.size()) {}

		CString(CString&& other) noexcept : str(std::move(other.str)) {}

		CString& operator=(const CString& other)
		{
			if (this == &other)
				return *this;

			return *ayr_construct(this, other);
		}

		CString& operator=(CString&& other) noexcept
		{
			if (this == &other)
				return *this;

			return *ayr_construct(this, std::move(other));
		}

		operator const char* () const { return data(); }

		operator char* () { return data(); }

		char& operator[] (c_size index) { return str[index]; }

		const char& operator[] (c_size index) const { return str[index]; }

		size_t size() const { return std::strlen(data()); }

		char* data() { return str.get(); }

		const char* data() const { return str.get(); }

		CString __str__() const { return *this; }

		size_t __hash__() const { return bytes_hash(data(), size()); }

		cmp_t __cmp__(const CString& other) const
		{
			for (size_t i = 0; str[i] || other.str[i]; ++i)
				if (str[i] != other.str[i])
					return str[i] - other.str[i];

			return 0;
		}

		std::unique_ptr<char[]> str;
	};

	inline CString cstr(int64_t value) { return std::to_string(value); }

	inline CString cstr(uint64_t value) { return std::to_string(value); }

	inline CString cstr(double value) { return std::to_string(value); }

	inline CString cstr(bool value) { return  ifelse(value, CString("true", 4), CString("false", 5)); }

	inline CString cstr(const char* str_) { return CString(str_, std::strlen(str_)); }
}
#endif // AYR_LAW_DETAIL_CSTRING_HPP