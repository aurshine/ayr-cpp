#ifndef AYR_BASE_CSTRING_HPP
#define AYR_BASE_CSTRING_HPP

#include <string>
#include <format>
#include <sstream>

#include "Buffer.hpp"
#include "hash.hpp"


namespace ayr
{
	constexpr c_size strlen(const char* str)
	{
		c_size len = 0;
		while (str[len]) ++len;
		return len;
	}

	template<size_t N>
	constexpr c_size strlen(const char(&str)[N]) { return N - 1; }

	class CString
	{
		using self = CString;

		const char* str;

		// 最高位1字节记录是否占有内存，0表示内存不属于该对象，1表示内存属于该对象
		// 剩余的63位记录字符串长度
		c_size length_and_owner_flag;

		// 最高位字节为1，x & OWNER_MASK == 1 表示内存属于该对象
		static constexpr c_size OWNER_MASK = 1ll << 63;
	public:
		friend class Buffer;

		constexpr CString() : str(""), length_and_owner_flag(OWNER_MASK) {}

		constexpr CString(const char* str_) : str(str_), length_and_owner_flag(strlen(str_)) {}

		template<size_t N>
		constexpr CString(const char(&str_)[N]) : str(str_), length_and_owner_flag(N - 1) {}

		constexpr CString(const char* str_, c_size len_, bool owner = false) : str(str_), length_and_owner_flag(ifelse(owner, len_ | OWNER_MASK, len_)) {}

		constexpr CString(const self& other) : str(other.str), length_and_owner_flag(other.size()) {}

		constexpr CString(self&& other) noexcept : str(other.str), length_and_owner_flag(other.length_and_owner_flag)
		{
			other.str = nullptr;
			other.length_and_owner_flag = 0;
		}

		~CString()
		{
			if (owner())
				ayr_delloc(const_cast<char*>(str));
			length_and_owner_flag = 0;
		}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);

			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);

			return *ayr_construct(this, std::move(other));
		}

		constexpr const char& operator[] (c_size index) const { return str[index]; }

		// 判断是否拥有内存
		constexpr bool owner() const { return length_and_owner_flag & OWNER_MASK; }

		// 获得字符串长度
		constexpr c_size size() const { return length_and_owner_flag & ~OWNER_MASK; }

		constexpr bool empty() const { return size() == 0; }

		constexpr const char* data() const { return str; }

		// 拷贝一个新的 CString 对象，并拥有内存
		self clone() const
		{
			c_size len = size();
			char* new_str = ayr_alloc<char>(len + 1);
			std::memcpy(new_str, str, len);
			new_str[len] = '\0';
			return self(new_str, len, true);
		}

		cmp_t __cmp__(const self& other) const { return std::strcmp(data(), other.data()); }

		bool __equals__(const self& other) const { return __cmp__(other) == 0; }

		bool __equals__(const char* other) const { return std::strcmp(data(), other) == 0; }

		size_t __hash__() const { return bytes_hash(data(), size()); }

		void __repr__(Buffer& buffer) const { buffer.append_bytes(data(), size()); }

		self __str__() const { return *this; }

		void __swap__(self& other)
		{
			std::swap(str, other.str);
			std::swap(length_and_owner_flag, other.length_and_owner_flag);
		}

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
			char* new_str = ayr_alloc<char>(s_size + o_size + 1);

			std::memcpy(new_str, data(), s_size);
			std::memcpy(new_str + s_size, other.data(), o_size);
			new_str[s_size + o_size] = '\0';
			return self(new_str, s_size + o_size, true);
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

			char* new_str = ayr_alloc<char>(len + 1);
			char* ptr = new_str;
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
			*ptr = '\0';
			return self(new_str, len, true);
		}

		template<IteratableU<self> Obj>
		static self cjoin(const Obj& elems)
		{
			c_size length = 0;
			for (const self& s : elems)
				length += s.size();

			char* new_str = ayr_alloc<char>(length + 1);
			char* ptr = new_str;
			for (const self& s : elems)
			{
				const char* s_ptr = s.data();
				while (*s_ptr)
				{
					*ptr = *s_ptr;
					++ptr, ++s_ptr;
				}
			}
			*ptr = '\0';
			return self(new_str, length, true);
		}
	};

	// owner string
	// 浅拷贝，并拥有str内存
	def ostr(const char* str, c_size len = -1)
	{
		if (len == -1) len = strlen(str);
		return CString(str, len, true);
	}

	def ostr(const std::string& str) { return CString(str.c_str(), str.size(), true); }

	def ostr(const std::string_view& str) { return CString(str.data(), str.size(), true); }

	// view string
	// 浅拷贝，不拥有str内存
	def vstr(const char* str, c_size len = -1)
	{
		if (len == -1) len = strlen(str);
		return CString(str, len, false);
	}

	def vstr(const std::string& str) { return CString(str.c_str(), str.size(), false); }

	def vstr(const std::string_view& str) { return CString(str.data(), str.size(), false); }

	// deep string
	// 深拷贝, 并拥有str内存
	def dstr(const char* str, c_size len = -1)
	{
		if (len == -1) len = strlen(str);
		char* new_str = ayr_alloc<char>(len + 1);
		std::memcpy(new_str, str, len);
		new_str[len] = '\0';
		return CString(new_str, len, true);
	}

	def dstr(const std::string& str) { return dstr(str.c_str(), str.size()); }

	def dstr(const std::string_view& str) { return dstr(str.data(), str.size()); }

	// 将任意类型转化为元字面量字符串
	template<typename T>
	der(CString) cstr_meta(const T& value)
	{
		std::string type_name = dtype(value);
		int s_len = type_name.size() + 22;
		char* s = ayr_alloc<char>(s_len);
#ifdef _MSC_VER
		sprintf_s(s, s_len, "<%s 0x%p>", type_name.c_str(), &value);
#else
		std::sprintf(s, "<%s 0x%p>", type_name.c_str(), &value);
#endif
		return ostr(s);
	}

	// 整型转换为 CString 对象
	der(CString) cstr_int(c_size value)
	{
		char* tmp = ayr_alloc<char>(32);
		sprintf_int(tmp, 32, value);
		return ostr(tmp);
	}

	// 浮点型转换为 CString 对象
	der(CString) cstr_float(double value)
	{
		char* tmp = ayr_alloc<char>(32);
		sprintf_float(tmp, 32, value);
		return ostr(tmp);
	}

	// 指针转换为 CString 对象
	der(CString) cstr_pointer(const void* value)
	{
		char* tmp = ayr_alloc<char>(32);
		sprintf_pointer(tmp, 32, value);
		return ostr(tmp, 32);
	}

	// nullptr 转换为 CString 对象
	der(CString) cstr(nullptr_t) { return "nullptr"; }

	// 将bool 转换为 CString 对象
	der(CString) cstr(bool value) { return ifelse(value, "true", "false"); }

	// 将char 转换为 CString 对象
	der(CString) cstr(char value) { return dstr(&value, 1); }

	// 将const char* 转换为 CString 对象
	der(CString) cstr(const char* value, c_size len = -1)
	{
		if (len == -1) len = strlen(value);
		return dstr(value, len);
	}

	// 将std::string 转换为 CString 对象
	der(CString) cstr(const std::string& value) { return dstr(value); }

	// 将std::string_view 转换为 CString 对象
	der(CString) cstr(const std::string_view& value) { return dstr(value); }

	// 将任意类型转换为 CString 对象
	template<typename T>
	der(CString) cstr(const T& value)
	{
		if constexpr (hasmethod(T, __str__))
			return value.__str__();
		else if constexpr (hasmethod(T, __repr__, std::declval<Buffer&>()))
		{
			Buffer buf;
			value.__repr__(buf);
			return dstr(buf.data(), buf.size());
		}
		else if constexpr (std::is_integral_v<T>)
			return cstr_int(value);
		else if constexpr (std::is_floating_point_v<T>)
			return cstr_float(value);
		else if constexpr (std::is_pointer_v<T>)
			return cstr_pointer(value);
		else
			return cstr_meta(value);
	}

	Buffer& operator<< (Buffer& buffer, const CString& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}
}

der(std::ostream&) operator<<(std::ostream& os, const ayr::CString& str) { return os.write(str.data(), str.size()); }

template<>
struct std::formatter<ayr::CString> : std::formatter<const char*>
{
	auto format(const ayr::CString& value, auto& ctx) const
	{
		return std::formatter<const char*>::format(value.data(), ctx);
	}
};

#endif // AYR_BASE_CSTRING_HPP