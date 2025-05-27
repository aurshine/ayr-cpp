#ifndef AYR_BASE_CSTRING_HPP
#define AYR_BASE_CSTRING_HPP

#include <string>
#include <format>
#include <sstream>

#include "Buffer.hpp"
#include "hash.hpp"


namespace ayr
{
	class CString
	{
		using self = CString;
	public:
		friend class Buffer;

		CString() : str(ayr_alloc<char>(1)) { std::memset(str, 0, 1); }

		CString(c_size len) : str(ayr_alloc<char>(len + 1)) { std::memset(str, 0, len + 1); }

		CString(const char* str_) : CString(str_, std::strlen(str_)) {}

		CString(const char* str_, c_size len_) : CString(len_) { std::memcpy(data(), str_, len_); }

		CString(const std::basic_string<char>& str_) : CString(str_.c_str(), str_.size()) {}

		CString(const CString& other) : CString(other.data(), other.size()) {}

		CString(CString&& other) noexcept : str(other.str) { other.str = nullptr; }

		~CString() { ayr_delloc(str); }

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

		char& operator[] (c_size index) { return str[index]; }

		const char& operator[] (c_size index) const { return str[index]; }

		c_size size() const { return std::strlen(data()); }

		bool empty() const { return str == nullptr || str[0] == '\0'; }

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

		self operator+(const self& other)
		{
			c_size s_size = size(), o_size = other.size();
			self ret(s_size + o_size);
			std::memcpy(ret.data(), data(), s_size);
			std::memcpy(ret.data() + s_size, other.data(), o_size);
			return ret;
		}

		self& operator+=(const self& other)
		{
			self res = *this + other;
			*this = std::move(res);
			return *this;
		}

		template<IteratableU<self> Obj>
		self join(const Obj& elems) const
		{
			if (empty()) return cjoin(elems);

			c_size len = 0, s_size = size();

			for (const self& str : elems)
				len += str.size() + s_size;
			if (len) len -= s_size;

			self ret(len);
			char* ptr = ret.data();
			for (auto it = elems.begin(); it != elems.end(); ++it)
			{
				c_size o_size = it->size();
				if (it != elems.begin())
				{
					std::memcpy(ptr, data(), s_size);
					ptr += s_size;
				}
				std::memcpy(ptr, it->data(), o_size);
				ptr += o_size;
			}
			return ret;
		}

		template<IteratableU<CString> Obj>
		static CString cjoin(const Obj& elems)
		{
			c_size length = 0;
			for (const CString& s : elems)
				length += s.size();
			CString result(length);
			char* ptr = result.data();
			for (const CString& s : elems)
			{
				const char* s_ptr = s.data();
				while (*s_ptr)
				{
					*ptr = *s_ptr;
					++ptr, ++s_ptr;
				}
			}

			return result;
		}
	private:
		char* str;
	};

	template<typename T>
	der(CString) meta_cstr(const T& value)
	{
		std::string type_name = dtype(value);
		int s_len = type_name.size() + 22;
		CString s(s_len);
#ifdef _MSC_VER
		sprintf_s(s.data(), s_len, "<%s 0x%p>", type_name.c_str(), &value);
#else
		std::sprintf(s.data(), "<%s 0x%p>", type_name.c_str(), &value);
#endif
		return s;
	}

	der(CString) cstr_int(c_size value)
	{
		CString tmp(32);
		sprintf_int(tmp.data(), 32, value);
		return tmp;
	}

	der(CString) cstr_float(double value)
	{
		CString tmp(32);
		sprintf_float(tmp.data(), 32, value);
		return tmp;
	}

	der(CString) cstr_pointer(const void* value)
	{
		CString tmp(32);
		sprintf_pointer(tmp.data(), 32, value);
		return tmp;
	}

	der(CString) cstr(bool value) { return ifelse(value, "true", "false"); }

	der(CString) cstr(char value) { return CString(&value, 1); }

	der(CString) cstr(const char* value) { return value; }

	der(CString) cstr(nullptr_t) { return "nullptr"; }

	der(CString) cstr(const std::string& value) { return CString(value.c_str(), value.size()); }

	der(CString) cstr(const std::string_view& value) { return CString(value.data(), value.size()); }

	der(CString) cstr(const CString& value) { return value; }

	template<typename T>
	der(CString) cstr(const T& value)
	{
		if constexpr (hasmethod(T, __str__))
			return value.__str__();
		else if constexpr (hasmethod(T, __repr__, std::declval<Buffer&>()))
		{
			Buffer buf;
			value.__repr__(buf);
			return CString(buf.data(), buf.size());
		}
		else if constexpr (std::is_integral_v<T>)
			return cstr_int(value);
		else if constexpr (std::is_floating_point_v<T>)
			return cstr_float(value);
		else if constexpr (std::is_pointer_v<T>)
			return cstr_pointer(value);
		else
			return meta_cstr(value);
	}

	Buffer& operator<< (Buffer& buffer, const CString& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}
}

der(std::ostream&) operator<<(std::ostream& os, const ayr::CString& str) { return os << str.data(); }

template<>
struct std::formatter<ayr::CString> : std::formatter<const char*>
{
	auto format(const ayr::CString& value, auto& ctx) const
	{
		return std::formatter<const char*>::format(value.data(), ctx);
	}
};

#endif // AYR_BASE_CSTRING_HPP