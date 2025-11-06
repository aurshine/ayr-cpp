#ifndef AYR_BASE_CSTRING_HPP
#define AYR_BASE_CSTRING_HPP

#include <format>
#include <sstream>
#include <string>

#include "Buffer.hpp"
#include "hash.hpp"

namespace ayr
{
	// 结尾带\0的c风格字符串
	class StringZero
	{
		union {
			char short_str[16];
			const char* long_str;
		};

		// 是否sso优化
		bool sso;
	public:
		constexpr StringZero() :StringZero("", 0) {}

		constexpr StringZero(const char* str, c_size len) : short_str()
		{
			if (len < 16)
			{
				std::copy(str, str + len, short_str);
				short_str[len] = 0;
				sso = true;
			}
			else
			{
				char* ptr = ayr_alloc<char>(len + 1);
				std::copy(str, str + len, ptr);
				ptr[len] = 0;
				long_str = ptr;
				sso = false;
			}
		}

		constexpr StringZero(const StringZero& other) : StringZero(other.c_str(), other.size()) {}

		constexpr c_size size() const
		{
			c_size length = 0;
			const char* ptr = c_str();
			while (ptr[length]) ++length;
			return length;
		}

		constexpr const char* c_str() const { return ifelse(sso, short_str, long_str); }

		constexpr operator const char* () const { return c_str(); }

		constexpr bool empty() const { return size() == 0; }

		constexpr const char* begin() const { return c_str(); }

		constexpr const char* end() const { return c_str() + size(); }

		void __repe__(Buffer& buffer) const { buffer << c_str(); }
	};

	constexpr c_size strlen(const char* str)
	{
		c_size len = 0;
		while (str[len]) ++len;
		return len;
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

		// 最高位字节为1，x & OWNER_MASK == 1 表示内存属于该对象，内存的生命周期随对象控制
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
		constexpr CString() : short_str(), owner_sso_length_flag(SSO_MASK) {}

		// 浅拷贝，根据owner参数决定是否占有内存，不使用sso优化
		constexpr CString(const char* str_, c_size len = -1, bool owner = false) : long_str(str_)
		{
			if (len == -1) len = ayr::strlen(str_);
			if (owner)
				owner_sso_length_flag = len | OWNER_MASK;
			else
				owner_sso_length_flag = len;
		}

		// 浅拷贝，不占有内存，不使用sso优化
		constexpr CString(const self& other) : CString(other.data(), other.size(), false) {}

		// 如果other是sso优化的，则直接赋值，否则代替内存占用
		constexpr CString(self&& other) noexcept
		{
			if (other.sso())
				std::copy(other.begin(), other.end(), short_str);
			else
				long_str = other.long_str;
			owner_sso_length_flag = other.owner_sso_length_flag;
			other.owner_sso_length_flag = 0;
		}

		constexpr ~CString()
		{
			// long_str的内存是char*分配的，需要转换为char*再释放
			if (owner())
				ayr_delloc(const_cast<char*>(long_str));
			owner_sso_length_flag = 0;
		}

		constexpr self& operator=(const self& other)
		{
			if (this == &other) return *this;
			this->~CString();

			this->long_str = other.data();
			this->owner_sso_length_flag = other.size();
			return *this;
		}

		constexpr self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			this->~CString();

			if (other.sso())
				std::copy(other.begin(), other.end(), short_str);
			else
				long_str = other.long_str;
			owner_sso_length_flag = other.owner_sso_length_flag;
			other.owner_sso_length_flag = 0;
			return *this;
		}

		// 字符串拼接
		constexpr self operator+(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			self res;

			char* ptr = res.assign_ptr(m_size + o_size);
			ptr = std::copy(begin(), end(), ptr);
			std::copy(other.begin(), other.end(), ptr);

			return res;
		}

		// 字符串拼接
		constexpr self operator+(const char& ch) const
		{
			c_size m_size = size();
			self res;
			char* ptr = res.assign_ptr(m_size + 1);
			std::copy(begin(), end(), ptr);
			ptr[m_size] = ch;
			return res;
		}

		// 字符串拼接
		constexpr self& operator+=(const self& other)
		{
			c_size m_size = size(), o_size = other.size();
			if (sso() && m_size + o_size <= SSO_SIZE)
			{
				std::copy(other.begin(), other.end(), short_str + m_size);
				owner_sso_length_flag = (m_size + o_size) | SSO_MASK;
			}
			else
			{
				self tmp = *this + other;
				*this = std::move(tmp);
			}
			return *this;
		}

		// 字符串拼接
		constexpr self& operator+=(const char& ch)
		{
			c_size m_size = size();
			if (sso() && m_size + 1 <= SSO_SIZE)
			{
				short_str[m_size] = ch;
				owner_sso_length_flag = (m_size + 1) | SSO_MASK;
			}
			else
			{
				self tmp = *this + ch;
				*this = std::move(tmp);
			}
			return *this;
		}

		// 复制字符串
		constexpr self operator*(c_size n) const
		{
			if (n <= 0 || empty()) return self();
			c_size m_size = size();

			self res;
			char* ptr = res.assign_ptr(n * m_size);
			for (c_size i = 0; i < n; ++i)
				ptr = std::copy(begin(), end(), ptr);
			return res;
		}

		// 判断是否拥有内存
		constexpr bool owner() const { return owner_sso_length_flag & OWNER_MASK; }

		// 判断是否使用sso优化
		constexpr bool sso() const { return owner_sso_length_flag & SSO_MASK; }

		// 判断是否只是视图
		constexpr bool viewer() const { return (owner_sso_length_flag & (OWNER_MASK | SSO_MASK)) == 0; }

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
		constexpr StringZero c_str() const { return StringZero(data(), size()); }

		// 获得起始迭代器
		constexpr const char* begin() const { return data(); }

		// 获得终止迭代器
		constexpr const char* end() const { return data() + size(); }

		// 判断是否包含子串other
		constexpr bool contains(const self& other) const { return index(other) != -1; }

		// 判断是否包含ch
		constexpr bool contains(const char& ch) const { return index(ch) != -1; }

		/*
		* @brief 从pos开始从前往后查找子串other的位置
		*
		* @param other 要查找的子串
		*
		* @param pos 开始查找的位置
		*
		* @return 所在下标，不存在返回-1
		*/
		constexpr c_size index(const self& other, c_size pos = 0) const
		{
			c_size m_size = size(), o_size = other.size();
			if (o_size > m_size - pos) return -1;

			while (pos <= m_size - o_size)
			{
				if (std::equal(other.begin(), other.end(), begin() + pos))
					return pos;
				++pos;
			}
			return -1;
		}

		/*
		* @brief 从pos开始从前往后查找子串ch的位置
		*
		* @param ch 要查找的字符
		*
		* @param pos 开始查找的位置
		*
		* @return 所在下标，不存在返回-1
		*/
		constexpr c_size index(const char& ch, c_size pos = 0) const
		{
			c_size m_size = size();

			while (pos < m_size)
			{
				if (at(pos) == ch)
					return pos;
				++pos;
			}
			return -1;
		}

		/*
		* @brief 从pos开始从后往前查找子串other的位置
		*
		* @param other 要查找的子串
		*
		* @param pos 开始查找的位置
		*
		* @return 所在下标，不存在返回-1
		*/
		constexpr c_size rindex(const self& other, c_size pos = -1) const
		{
			c_size m_size = size(), o_size = other.size();
			if (pos < 0 || pos > m_size - o_size)
				pos = m_size - o_size;

			while (pos >= 0)
			{
				if (std::equal(other.begin(), other.end(), begin() + pos))
					return pos;
				--pos;
			}
			return -1;
		}

		/*
		* @brief 从pos开始从后往前查找子串ch的位置
		*
		* @param ch 要查找的字符
		*
		* @param pos 开始查找的位置
		*
		* @return 所在下标，不存在返回-1
		*/
		constexpr c_size rindex(const char& ch, c_size pos = -1) const
		{
			if (pos < 0 || pos > size() - 1) pos = size() - 1;
			while (pos >= 0)
			{
				if (at(pos) == ch)
					return pos;
				--pos;
			}
			return -1;
		}

		// 获取other在字符串中出现的次数
		constexpr c_size count(const self& other, c_size pos = 0) const
		{
			c_size m_size = size(), o_size = other.size();
			c_size count = 0;
			if (o_size > m_size - pos) return 0;

			while (pos <= m_size - o_size)
			{
				if (std::equal(other.begin(), other.end(), begin() + pos))
				{
					++count;
					pos += o_size;
				}
				else
					++pos;
			}
			return count;
		}

		// 获取ch在字符串中出现的次数
		constexpr c_size count(const char& ch, c_size pos = 0) const
		{
			c_size m_size = size(), count = 0;
			while (pos < m_size)
			{
				if (at(pos) == ch)
					++count;
				++pos;
			}
			return count;
		}

		// 拷贝一个新的 CString 对象，并拥有内存
		constexpr self clone() const
		{
			self res;
			char* ptr = res.assign_ptr(size());
			std::copy(begin(), end(), ptr);
			return res;
		}

		// 字符串切片，[start, end)，浅拷贝
		constexpr self vslice(c_size start, c_size end) const
		{
			self res;
			// 越界检查
			if (end > start)
			{
				// 可以sso优化
				if (end - start <= SSO_SIZE)
				{
					std::copy(begin() + start, begin() + end, res.short_str);
					res.owner_sso_length_flag = (end - start) | SSO_MASK;
				}
				else
				{
					res.long_str = data() + start;
					res.owner_sso_length_flag = end - start;
				}
			}

			return res;
		}

		// 字符串切片，[start, size())，浅拷贝
		constexpr self vslice(c_size start) const { return vslice(start, size()); }

		// 字符串切片，[start, end)，深拷贝
		constexpr self slice(c_size start, c_size end) const
		{
			self res = vslice(start, end);
			// 视图转为深拷贝
			if (res.viewer())
				return res.clone();
			else
				return res;
		}

		// 字符串切片，[start, size())，深拷贝
		constexpr self slice(c_size start) const { return slice(start, size()); }

		// 判断是否以prefix开头
		constexpr bool startswith(const self& preifx) const
		{
			if (preifx.size() > size()) return false;
			return std::equal(preifx.begin(), preifx.end(), begin());
		}

		// 判断是否以suffix结尾
		constexpr bool endswith(const self& suffix) const
		{
			if (suffix.size() > size()) return false;
			return std::equal(suffix.begin(), suffix.end(), begin() + size() - suffix.size());
		}

		// 判断是否为空字符串
		constexpr bool isspace() const
		{
			for (const char& ch : *this)
				if (ch < 9 || ch > 13 || (ch != '\0' && ch != ' '))
					return false;
			return true;
		}

		// 判断是否为数字字符串
		constexpr bool isdigit() const
		{
			for (const char& ch : *this)
				if (ch < '0' || ch > '9')
					return false;
			return true;
		}

		// 判断是否为字母字符串
		constexpr bool isalpha() const
		{
			for (const char& ch : *this)
				if ((ch < 'a' || ch > 'z') && (ch < 'A' || ch > 'Z'))
					return false;
			return true;
		}

		// 判断是否为大写字符串
		constexpr bool isupper() const
		{
			for (const char& ch : *this)
				if (ch < 'A' || ch > 'Z')
					return false;
			return true;
		}

		// 判断是否为小写字符串
		constexpr bool islower() const
		{
			for (const char& ch : *this)
				if (ch < 'a' || ch > 'z')
					return false;
			return true;
		}

		// 返回新的大写字符串
		constexpr self upper() const
		{
			c_size m_size = size();
			self res;

			char* ptr = res.assign_ptr(m_size);
			for (const char& ch : *this)
			{
				if (ch >= 'a' && ch <= 'z')
					*ptr = ch - 'a' + 'A';
				else
					*ptr = ch;
				++ptr;
			}
			return res;
		}

		// 返回新的小写字符串
		constexpr self lower() const
		{
			c_size m_size = size();
			self res;

			char* ptr = res.assign_ptr(m_size);
			for (const char& ch : *this)
			{
				if (ch >= 'A' && ch <= 'Z')
					*ptr = ch - 'A' + 'a';
				else
					*ptr = ch;
				++ptr;
			}
			return res;
		}

		// 通过*this，连接可迭代对象中的字符串
		template<IteratableU<self> Obj>
		constexpr self join(const Obj& elems) const
		{
			if (empty()) return cjoin(elems);

			c_size res_size = 0, m_size = size();
			// 计算结果字符串长度
			for (const self& str : elems)
				res_size += str.size() + m_size;

			self res;
			if (res_size - m_size > 0)
			{
				char* ptr = res.assign_ptr(res_size - m_size);
				// 是否已经复制了第一个元素
				bool first_flag = false;
				for (const self& elem : elems)
				{
					if (first_flag)
						ptr = std::copy(begin(), end(), ptr);

					ptr = std::copy(elem.begin(), elem.end(), ptr);
					first_flag = true;
				}
			}
			return res;
		}

		template<IteratableU<self> Obj>
		constexpr static self cjoin(const Obj& elems)
		{
			c_size res_size = 0;
			for (const self& elem : elems)
				res_size += elem.size();

			self res;
			if (res_size > 0)
			{
				char* ptr = res.assign_ptr(res_size);
				for (const self& elem : elems)
					ptr = std::copy(elem.begin(), elem.end(), ptr);
			}
			return res;
		}

		constexpr cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0; i < m_size && i < o_size; ++i)
				if (at(i) != other.at(i))
					return at(i) - other.at(i);
			return m_size - o_size;
		}

		constexpr bool __equals__(const self& other) const
		{
			if (size() != other.size()) return false;
			return std::equal(begin(), end(), other.begin());
		}

		constexpr bool __equals__(const char* other) const
		{
			if (size() != ayr::strlen(other)) return false;
			return std::equal(begin(), end(), other);
		}

		size_t __hash__() const { return bytes_hash(data(), size()); }

		void __repr__(Buffer& buffer) const { buffer.append_bytes(data(), size()); }

		constexpr self __str__() const { return *this; }

		constexpr bool operator> (const self& other) const { return __cmp__(other) > 0; }

		constexpr bool operator< (const self& other) const { return __cmp__(other) < 0; }

		constexpr bool operator>= (const self& other) const { return __cmp__(other) >= 0; }

		constexpr bool operator<= (const self& other) const { return __cmp__(other) <= 0; }

		constexpr bool operator== (const self& other) const { return __cmp__(other) == 0; }

		constexpr bool operator!= (const self& other) const { return __cmp__(other) != 0; }

		constexpr bool operator== (const char* other) const { return __equals__(other); }

		constexpr bool operator!= (const char* other) const { return __equals__(other); }
	private:
		/*
		* @brief 根据length分配内存，返回首地址
		*
		* @details 若length <= SSO_SIZE, 则分配sso优化内存，否则分配堆内存
		*
		* @param length 字符串长度
		*
		* @return char* 首地址
		*/
		constexpr char* assign_ptr(c_size length)
		{
			char* ptr = nullptr;
			if (length <= SSO_SIZE)
			{
				ptr = short_str;
				owner_sso_length_flag = length | SSO_MASK;
			}
			else
			{
				ptr = ayr_alloc<char>(length);
				long_str = ptr;
				owner_sso_length_flag = length | OWNER_MASK;
			}

			return ptr;
		}
	};

	// owner string
	// 浅拷贝，并拥有str内存
	constexpr def ostr(const char* str, c_size len = -1) { return CString(str, len, true); }

	// view string
	// 浅拷贝，不拥有str内存
	constexpr  def vstr(const char* str, c_size len = -1) { return CString(str, len, false); }

	// 浅拷贝，不拥有str内存
	constexpr  def vstr(const std::string& str) { return CString(str.c_str(), str.size(), false); }

	// 浅拷贝，不拥有str内存
	constexpr  def vstr(const std::string_view& str) { return CString(str.data(), str.size(), false); }

	// deep string
	// 深拷贝, 并拥有str内存
	constexpr def dstr(const char* str, c_size len = -1) { return vstr(str, len).clone(); }

	// 深拷贝, 并拥有str内存
	constexpr def dstr(const std::string& str) { return vstr(str.c_str(), str.size()).clone(); }

	// 深拷贝, 并拥有str内存
	constexpr def dstr(const std::string_view& str) { return vstr(str.data(), str.size()).clone(); }

	// nullptr 转换为 CString 对象
	constexpr der(CString) cstr(nullptr_t) { return vstr("nullptr", 7); }

	// 将bool 转换为 CString 对象
	constexpr der(CString) cstr(bool value) { return ifelse(value, vstr("true", 4), vstr("false", 5)); }

	// 将char 转换为 CString 对象
	constexpr der(CString) cstr(char value) { return dstr(&value, 1); }

	// 将const char* 转换为 CString 对象
	constexpr der(CString) cstr(const char* value, c_size len = -1) { return dstr(value, len); }

	// 将std::string 转换为 CString 对象
	constexpr der(CString) cstr(const std::string& value) { return dstr(value); }

	// 将std::string_view 转换为 CString 对象
	constexpr der(CString) cstr(const std::string_view& value) { return dstr(value); }

	// 将任意类型转换为 CString 对象
	template<typename T>
	constexpr der(CString) cstr(const T& value)
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