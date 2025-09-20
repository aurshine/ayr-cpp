#ifndef AYR_BASE_CSTRING_HPP
#define AYR_BASE_CSTRING_HPP

#include <functional>
#include <format>
#include <sstream>
#include <string>

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

	constexpr void memcpy(void* dst, const void* src, c_size size)
	{
		for (c_size i = 0; i < size; ++i)
			reinterpret_cast<char*>(dst)[i] = reinterpret_cast<const char*>(src)[i];
	}

	/**
	 * CString 是一个支持 SSO 优化与手动内存管理的高性能字符串类。
	 *
	 * ⚠️ 构造 CString 时请优先使用辅助函数：
	 *   - `ostr(str)`：拥有内存
	 *   - `vstr(str)`：只读视图
	 *   - `dstr(str)`：深拷贝
	 *   - `cstr(value)`：自动根据类型构造 CString
	 *
	 * 🚫 不建议直接使用构造函数，如 `CString(str, len, owner)`，易产生错误的内存管理行为。
	 */
	class CString
	{
		using self = CString;

		// 最高位字节为1，x & OWNER_MASK == 1 表示内存属于该对象
		static constexpr c_size OWNER_MASK = 1ll << 63;

		// 最高位字节为1，x & SSO_MASK == 1 表示是sso优化
		static constexpr c_size SSO_MASK = 1ll << 62;

		// sso优化的字符串长度
		static constexpr c_size SSO_SIZE = 16;

		// 最高位1字节记录是否占有内存，0表示内存不属于该对象，1表示内存属于该对象
		// 次高位1字符记录是否是sso优化，0表示不是sso优化，1表示是sso优化
		// 剩余的62位记录字符串长度
		c_size owner_sso_length_flag;

		union {
			// 长字符串，使用堆内存
			const char* long_str;

			// 短字符串，使用sso优化
			char short_str[SSO_SIZE];
		};
	public:
		// 空字符串，不占有内存，不使用sso优化
		constexpr CString() : long_str(""), owner_sso_length_flag(0) {}

		// 浅拷贝，根据owner参数决定是否占有内存，不使用sso优化
		constexpr CString(const char* str_, c_size len = -1, bool owner = false) : long_str(str_)
		{
			if (len == -1) len = strlen(str_);
			if (owner)
				owner_sso_length_flag = len | OWNER_MASK;
			else
				owner_sso_length_flag = len;
		}

		// 浅拷贝，根据owner参数决定是否占有内存，不使用sso优化
		template<size_t N>
		constexpr CString(const char(&str_)[N], bool owner = false) : CString(str_, N - 1, owner) {}

		// 浅拷贝，不占有内存，不使用sso优化
		constexpr CString(const self& other) : CString(other.data(), other.size(), false) {}

		constexpr CString(self&& other) noexcept
		{
			if (other.sso())
				memcpy(short_str, other.short_str, SSO_SIZE);
			else
				long_str = other.long_str;
			owner_sso_length_flag = other.owner_sso_length_flag;
			other.owner_sso_length_flag = 0;
		}

		~CString()
		{
			if (owner())
				ayr_delloc(const_cast<char*>(long_str));
			owner_sso_length_flag = 0;
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

		// 判断是否拥有内存
		constexpr bool owner() const { return owner_sso_length_flag & OWNER_MASK; }

		// 判断是否使用sso优化
		constexpr bool sso() const { return owner_sso_length_flag & SSO_MASK; }

		// 获得字符串第index个字符
		constexpr const char& at(c_size index) const { return *(data() + index); }

		// 获得字符串第index个字符，允许负索引
		constexpr const char& operator[] (c_size index) const { return at(neg_index(index, size())); }

		// 获得字符串长度
		constexpr c_size size() const { return owner_sso_length_flag & ~(OWNER_MASK | SSO_MASK); }

		// 判断是否为空字符串
		constexpr bool empty() const { return size() == 0; }

		// 获得原始字符串首地址，不保证以\0结尾
		constexpr const char* data() const { return ifelse(sso(), short_str, long_str); }

		// 返回c风格字符串
		std::string c_str() const { return std::string(data(), size()); }

		// 拷贝一个新的 CString 对象，并拥有内存
		self clone() const
		{
			return sso_assign(size(), [&](char*& ptr) {
				std::memcpy(ptr, this->data(), this->size());
				});
		}

		// 字符串切片，[start, end)，浅拷贝
		self vslice(c_size start, c_size end) const { return self(data() + start, end - start, false); }

		// 字符串切片，[start, size())，浅拷贝
		self vslice(c_size start) const { return vslice(start, size()); }

		// 字符串切片，[start, end)，深拷贝
		self slice(c_size start, c_size end) const { return vslice(start, end).clone(); }

		// 字符串切片，[start, size())，深拷贝
		self slice(c_size start) const { return slice(start, size()); }

		// 判断是否以prefix开头
		bool startswith(const self& preifx) const
		{
			c_size m_size = size(), p_size = preifx.size();
			if (p_size > m_size) return false;
			return std::memcmp(data(), preifx.data(), p_size) == 0;
		}

		// 判断是否以suffix结尾
		bool endswith(const self& suffix) const
		{
			c_size m_size = size(), s_size = suffix.size();
			if (s_size > m_size) return false;
			return std::memcmp(data() + m_size - s_size, suffix.data(), s_size) == 0;
		}

		// 通过*this，连接可迭代对象中的字符串
		template<IteratableU<self> Obj>
		self join(const Obj& elems) const
		{
			if (empty()) return cjoin(elems);

			c_size len = 0, s_size = size();

			for (const self& str : elems)
				len += str.size() + s_size;
			if (len) len -= s_size;

			return sso_assign(len, [&](char*& ptr) {
				for (auto it = elems.begin(); it != elems.end(); ++it)
				{
					c_size o_size = it->size();
					if (it != elems.begin())
					{
						std::memcpy(ptr, this->data(), s_size);
						ptr += s_size;
					}
					std::memcpy(ptr, it->data(), o_size);
					ptr += o_size;
				}
				});
		}

		template<IteratableU<self> Obj>
		static self cjoin(const Obj& elems)
		{
			c_size len = 0;
			for (const self& s : elems)
				len += s.size();

			return sso_assign(len, [&](char*& ptr) {
				for (const self& s : elems)
				{
					c_size s_size = s.size();
					std::memcpy(ptr, s.data(), s_size);
					ptr += s_size;
				}
				});
		}

		const char* begin() const { return data(); }

		const char* end() const { return data() + size(); }

		cmp_t __cmp__(const self& other) const
		{
			c_size min_size = std::min(size(), other.size());
			int cmp = std::memcmp(data(), other.data(), min_size);
			if (cmp) return cmp;
			return size() - other.size();
		}

		bool __equals__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size) return false;
			return std::memcmp(data(), other.data(), m_size) == 0;
		}

		bool __equals__(const char* other) const
		{
			c_size m_size = size(), o_size = strlen(other);
			if (m_size != o_size) return false;
			return std::memcmp(data(), other, m_size) == 0;
		}

		size_t __hash__() const { return bytes_hash(data(), size()); }

		void __repr__(Buffer& buffer) const { buffer.append_bytes(data(), size()); }

		self __str__() const { return this->clone(); }

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

			return sso_assign(s_size + o_size, [&](char*& ptr) {
				std::memcpy(ptr, this->data(), s_size);
				std::memcpy(ptr + s_size, other.data(), o_size);
				});
		}

		self& operator+=(const self& other)
		{
			if (sso() && size() + other.size() <= SSO_SIZE)
			{
				std::memcpy(short_str + size(), other.data(), other.size());
				owner_sso_length_flag = (size() + other.size()) | SSO_MASK;
			}
			else
			{
				self res = *this + other;
				*this = std::move(res);
			}

			return *this;
		}

	private:
		// 一个字节一个字节的生成新的字符串
		// 根据len参数决定是否使用sso优化
		static self sso_assign(c_size len, std::function<void(char*&)> fn)
		{
			self new_str;
			char* tmp = nullptr, * ptr = nullptr;
			if (len > SSO_SIZE)
				ptr = tmp = ayr_alloc<char>(len);
			else
				ptr = new_str.short_str;

			fn(ptr);

			if (len > SSO_SIZE)
			{
				new_str.long_str = tmp;
				new_str.owner_sso_length_flag = len | OWNER_MASK;
			}
			else
			{
				new_str.owner_sso_length_flag = len | SSO_MASK;
			}
			return new_str;
		}
	};

	// owner string
	// 浅拷贝，并拥有str内存
	def ostr(const char* str, c_size len = -1) { return CString(str, len, true); }

	def ostr(const std::string& str) { return CString(str.c_str(), str.size(), true); }

	def ostr(const std::string_view& str) { return CString(str.data(), str.size(), true); }

	// view string
	// 浅拷贝，不拥有str内存
	def vstr(const char* str, c_size len = -1) { return CString(str, len, false); }

	def vstr(const std::string& str) { return CString(str.c_str(), str.size(), false); }

	def vstr(const std::string_view& str) { return CString(str.data(), str.size(), false); }

	// deep string
	// 深拷贝, 并拥有str内存
	def dstr(const char* str, c_size len = -1) { return vstr(str, len).clone(); }

	def dstr(const std::string& str) { return vstr(str.c_str(), str.size()).clone(); }

	def dstr(const std::string_view& str) { return vstr(str.data(), str.size()).clone(); }

	/*
	* @brief 尝试取代buffer的内存构造 CString 对象
	*
	* @param buffer 待转换的 Buffer 对象
	*/
	def from_buffer(Buffer&& buffer) -> CString
	{
		if (buffer.begin() == buffer.peek())
		{
			CString res = ostr(buffer.peek(), buffer.readable_size());
			buffer.detach();
			return res;
		}
		return dstr(buffer.peek(), buffer.readable_size());
	}

	// nullptr 转换为 CString 对象
	der(CString) cstr(nullptr_t) { return vstr("nullptr", 7); }

	// 将bool 转换为 CString 对象
	der(CString) cstr(bool value) { return ifelse(value, vstr("true", 4), vstr("false", 5)); }

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
		else
		{
			Buffer buffer;
			buffer << value;
			return from_buffer(std::move(buffer));
		}
	}

	Buffer& operator<< (Buffer& buffer, const CString& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}
}

der(std::ostream&) operator<<(std::ostream& os, const ayr::CString& str) { return os.write(str.data(), str.size()); }

template<>
struct std::formatter<ayr::CString> : std::formatter<std::string_view>
{
	auto format(const ayr::CString& value, auto& ctx) const
	{
		return std::formatter<std::string_view>::format(std::string_view(value.data(), value.size()), ctx);
	}
};

#endif // AYR_BASE_CSTRING_HPP