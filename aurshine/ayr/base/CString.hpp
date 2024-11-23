#ifndef AYR_DETAIL_CSTRING_HPP
#define AYR_DETAIL_CSTRING_HPP

#include <cstring>
#include <string>
#include <format>

#include <ayr/base/hash.hpp>
#include <ayr/base/ayr_memory.hpp>


namespace ayr
{
	class CString
	{
		using self = CString;
	public:
		CString() : str(ayr_alloc<char>(1)) { std::memset(str, 0, 1); }

		CString(c_size len) : str(ayr_alloc<char>(len + 1)) { std::memset(str, 0, len + 1); }

		CString(const char* str_) : CString(str_, std::strlen(str_)) {}

		CString(const char* str_, c_size len_) : CString(len_) { std::memcpy(data(), str_, len_); }

		CString(const std::basic_string<char>& str_) : CString(str_.c_str(), str_.size()) {}

		CString(const CString& other) : CString(other.data(), other.size()) {}

		CString(CString&& other) noexcept : str(other.str) { other.str = nullptr; }

		CString& operator=(const CString& other)
		{
			if (this == &other) return *this;
			ayr_delloc(str);

			return *ayr_construct(this, other);
		}

		CString& operator=(CString&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_delloc(str);

			return *ayr_construct(this, std::move(other));
		}

		operator const char* () const { return data(); }

		operator char* () { return data(); }

		operator bool() { return size() != 0; }

		char& operator[] (c_size index) { return str[index]; }

		const char& operator[] (c_size index) const { return str[index]; }

		size_t size() const { return std::strlen(data()); }

		char* data() { return str; }

		const char* data() const { return str; }

		CString __str__() const { return *this; }

		size_t __hash__() const { return bytes_hash(data(), size()); }

		cmp_t __cmp__(const self& other) const { return std::strcmp(data(), other.data()); }

		bool __equals__(const self& other) const { return __cmp__(other) == 0; }

		bool __equals__(const char* other) const { return std::strcmp(data(), other) == 0; }

		bool operator> (const self& other) const { return __cmp__(other) > 0; }

		bool operator< (const self& other) const { return __cmp__(other) < 0; }

		bool operator>= (const self& other) const { return __cmp__(other) >= 0; }

		bool operator<= (const self& other) const { return __cmp__(other) <= 0; }

		bool operator== (const self& other) const { return __cmp__(other) == 0; }

		bool operator!= (const self& other) const { return __cmp__(other) != 0; }

		bool operator== (const char* other) const { return __equals__(other); }

		bool operator!= (const char* other) const { return __equals__(other); }
	private:
		char* str;
	};

	template<AyrPrintable T>
	inline CString _cstr_impl(const T& value) { return value.__str__(); }

	template<StdPrintable T>
	inline CString _cstr_impl(const T& value)
	{
		std::stringstream stream;
		stream << value;
		return stream.str();
	}

	template<Printable T>
	inline CString cstr(const T& value) { return _cstr_impl(value); }

	inline const char* stdstr(const CString& value) { return value.data(); }
}

template<>
struct std::formatter<ayr::CString> : std::formatter<const char*>
{
	auto format(const ayr::CString& value, auto& ctx) const
	{
		return std::formatter<const char*>::format(value.data(), ctx);
	}
};

#endif // AYR_DETAIL_CSTRING_HPP