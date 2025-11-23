#ifndef AYR_BASE_CODEC_ASTRING_HPP
#define AYR_BASE_CODEC_ASTRING_HPP

#include "UniCodec.hpp"
#include "../Array.hpp"

namespace ayr
{
	class Atring : public Object<Atring>
	{
		using self = Atring;

		using super = Object<Atring>;

		// 最高位字节为1，x & OWNER_MASK == 1 表示内存属于该对象
		static constexpr c_size OWNER_MASK = 1ll << 63;

		// 最高位字节为1，x & SSO_MASK == 1 表示是sso优化
		static constexpr c_size SSO_MASK = 1ll << 62;

		// sso优化的字符串长度
		static constexpr c_size SSO_SIZE = 6;

		// 最高位1字节记录是否占有内存，0表示内存不属于该对象，1表示内存属于该对象
		// 次高位1字符记录是否是sso优化，0表示不是sso优化，1表示是sso优化
		// 剩余的62位记录字符串长度
		c_size owner_sso_length_flag;

		union {
			// 长字符串，使用堆内存
			const AChar* long_str;

			// 短字符串，使用sso优化
			AChar short_str[SSO_SIZE];
		};

		// 使用Codec解码bytes，创建字符串
		template<UniCodec Codec>
		constexpr Atring(const CString& bytes, const Codec& codec) : Atring()
		{
			c_size len = decode_size(bytes, codec);
			AChar* ptr = assign_ptr(len);
			codec.decode(ptr, len, bytes);
		}
	public:
		// 默认构造函数，创建一个空字符串，使用sso优化
		constexpr Atring() : owner_sso_length_flag(0 | SSO_MASK), short_str() {}

		constexpr Atring(const AChar& ch) : short_str()
		{
			short_str[0] = ch;
			owner_sso_length_flag = 1 | SSO_MASK;
		}

		// 如果other是sso优化，深拷贝
		// 否则浅拷贝
		constexpr Atring(const self& other) : short_str()
		{
			if (other.sso())
			{
				std::copy(other.begin(), other.end(), short_str);
				owner_sso_length_flag = other.owner_sso_length_flag;
			}
			else
			{
				long_str = other.long_str;
				owner_sso_length_flag = other.size();
			}
		}

		constexpr Atring(self&& other) noexcept : short_str()
		{
			if (other.sso())
				std::copy(other.begin(), other.end(), short_str);
			else
				long_str = other.long_str;

			owner_sso_length_flag = other.owner_sso_length_flag;
			other.owner_sso_length_flag = 0;
		}

		constexpr ~Atring()
		{
			// long_str的内存是AChar*分配的，需要转换为AChar*再释放
			if (owner())
				ayr_desloc(const_cast<AChar*>(long_str));
			owner_sso_length_flag = 0;
		}

		constexpr self& operator=(const self& other)
		{
			if (this == &other) return *this;
			this->~Atring();

			if (other.sso())
			{
				std::copy(other.begin(), other.end(), short_str);
				owner_sso_length_flag = other.owner_sso_length_flag;
			}
			else
			{
				long_str = other.long_str;
				owner_sso_length_flag = other.size();
			}
			return *this;
		}

		constexpr self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			this->~Atring();

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

			AChar* ptr = res.assign_ptr(m_size + o_size);
			ptr = std::copy(begin(), end(), ptr);
			std::copy(other.begin(), other.end(), ptr);

			return res;
		}

		// 字符串拼接
		constexpr self operator+(const AChar& ch) const
		{
			c_size m_size = size();
			self res;
			AChar* ptr = res.assign_ptr(m_size + 1);
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
		constexpr self& operator+=(const AChar& ch)
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
			AChar* ptr = res.assign_ptr(n * m_size);
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

		// 获得字符串长度
		constexpr c_size size() const { return owner_sso_length_flag & ~(OWNER_MASK | SSO_MASK); }

		// 判断是否为空字符串
		constexpr bool empty() const { return size() == 0; }

		// 获得字符串第index个字符
		constexpr const AChar& at(c_size index) const { return *(data() + index); }

		// 获得字符串第index个字符, 允许负索引
		constexpr const AChar& operator[](c_size index) const { return at(neg_index(index, size())); }

		// 获得起始迭代器
		constexpr const AChar* begin() const { return data(); }

		// 获得终止迭代器
		constexpr const AChar* end() const { return data() + size(); }

		// 判断是否包含子串other
		constexpr bool contains(const self& other) const { return index(other) != -1; }

		// 判断是否包含ch
		constexpr bool contains(const AChar& ch) const { return index(ch) != -1; }

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
		constexpr c_size index(const AChar& ch, c_size pos = 0) const
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
		constexpr c_size rindex(const AChar& ch, c_size pos = -1) const
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
		constexpr c_size count(const AChar& ch, c_size pos = 0) const
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

		// 拷贝一个新的 Atring 对象，并拥有内存
		constexpr self clone() const
		{
			self res;
			AChar* ptr = res.assign_ptr(size());
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
			for (const AChar& ch : *this)
				if (!ch.isspace())
					return false;
			return true;
		}

		// 判断是否为数字字符串
		constexpr bool isdigit() const
		{
			for (const AChar& ch : *this)
				if (!ch.isdigit())
					return false;
			return true;
		}

		// 判断是否为字母字符串
		constexpr bool isalpha() const
		{
			for (const AChar& ch : *this)
				if (!ch.isalpha())
					return false;
			return true;
		}

		// 判断是否为大写字符串
		constexpr bool isupper() const
		{
			for (const AChar& ch : *this)
				if (!ch.isupper())
					return false;
			return true;
		}

		// 判断是否为小写字符串
		constexpr bool islower() const
		{
			for (const AChar& ch : *this)
				if (!ch.islower())
					return false;
			return true;
		}

		// 返回新的大写字符串
		constexpr self upper() const
		{
			c_size m_size = size();
			self res;

			AChar* ptr = res.assign_ptr(m_size);
			for (const AChar& ch : *this)
			{
				*ptr = ch.upper();
				++ptr;
			}
			return res;
		}

		// 返回新的小写字符串
		constexpr self lower() const
		{
			c_size m_size = size();
			self res;

			AChar* ptr = res.assign_ptr(m_size);
			for (const AChar& ch : *this)
			{
				*ptr = ch.lower();
				++ptr;
			}
			return res;
		}

		// 去除两端空白，浅拷贝
		constexpr self strip() const
		{
			c_size l = 0, r = size();
			auto begin_it = begin();
			// 从左往右找第一个非空字符
			while (l < r && (begin_it + l)->isspace()) ++l;
			// 从右往左找第一个非空字符
			while (l < r && (begin_it + r - 1)->isspace()) --r;
			return vslice(l, r);
		}

		// 去除两端pattern，浅拷贝
		constexpr self strip(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			// 从左往右找第一个非pattern字符
			while (vslice(l).startswith(pattern)) l += p_size;
			// 从右往左找第一个非pattern字符
			while (vslice(l, r).endswith(pattern)) r -= p_size;
			return vslice(l, r);
		}

		// 去除左侧空白，浅拷贝
		self lstrip() const
		{
			c_size l = 0, r = size();
			auto begin_it = begin();
			// 从左往右找第一个非空字符
			while (l < r && (begin_it + l)->isspace()) ++l;
			return vslice(l, r);
		}

		// 去除左侧pattern，浅拷贝
		constexpr self lstrip(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			// 从左往右找第一个非pattern字符
			while (vslice(l).startswith(pattern)) l += p_size;
			return vslice(l, r);
		}

		// 去除右侧空白，浅拷贝
		constexpr self rstrip() const
		{
			c_size l = 0, r = size();
			auto begin_it = begin();
			// 从右往左找第一个非空字符
			while (l < r && (begin_it + r - 1)->isspace()) --r;
			return vslice(l, r);
		}

		// 去除右侧pattern，浅拷贝
		constexpr self rstrip(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			// 从右往左找第一个非pattern字符
			while (vslice(l, r).endswith(pattern)) r -= p_size;
			return vslice(l, r);
		}

		// 根据this拼接迭代器对象
		template<IteratableU<self> Obj>
		constexpr self join(const Obj& elems) const
		{
			if (empty()) return ajoin(elems);

			c_size m_size = size(), res_size = 0;
			// 计算结果字符串长度
			for (const self& elem : elems)
				res_size += elem.size() + m_size;

			self res;
			if (res_size - m_size > 0)
			{
				AChar* ptr = res.assign_ptr(res_size - m_size);
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

		/*
		* @brief 替换old_字符串为new_字符串，最多替换maxreplace次
		*
		* @param old_ 被替换的字符串
		*
		* @param new_ 替换后的字符串, 空串直接返回this
		*
		* @param maxreplace 最大替换次数, -1表示无限制
		*
		* @return self 替换后的字符串
		*/
		constexpr self replace(const self& old_, const self& new_, c_size maxreplace = -1) const
		{
			c_size m_size = size(), old_size = old_.size(), new_size = new_.size();
			if (old_size == 0) return *this;

			c_size num_old = 0;
			// 找old字符串的数量
			for (c_size pos = 0; pos < m_size; pos += old_size)
			{
				pos = index(old_, pos);
				if (pos == -1 || ++num_old == maxreplace)
					break;
			}

			if (num_old == 0) return *this;

			self res;
			AChar* ptr = res.assign_ptr(m_size + (new_size - old_size) * num_old);

			// l 为上一个old的结尾
			// pos 为当前old的开头
			for (c_size l = 0, pos = 0; pos < m_size; pos += old_size)
			{
				if (num_old--)
				{
					l = pos;
					pos = index(old_, pos);
					ptr = std::copy(begin() + l, begin() + pos, ptr);
					ptr = std::copy(new_.begin(), new_.end(), ptr);
				}
				else
				{
					ptr = std::copy(begin() + pos, end(), ptr);
					break;
				}
			}
			return res;
		}

		/*
		* @brief 根据空白符切分字符串,数组里的元素浅拷贝
		*
		* @details 空白符包括空格、制表符、换行符、回车符
		*
		* @param maxsplit 最大切分次数, -1表示无限制
		*
		* @return Array<self> 切分后的字符串数组
		*/
		Array<self> split(c_size maxsplit = -1) const
		{
			c_size m_size = size();
			c_size n = 0;
			for (c_size i = 0; i < m_size; ++i)
				if (at(i).isspace() && (i == 0 || !at(i - 1).isspace()))
					++n;

			if (maxsplit != -1 && maxsplit < n)
				n = maxsplit;
			if (n == 0) return { *this };

			Array<self> res(n + 1);
			c_size l = 0, pos = 0, i = 0;
			while (i <= n)
			{
				l = pos;
				if (i == n)
					pos = m_size;
				else
					while (pos < m_size && !at(pos).isspace())
						++pos;
				res[i++] = vslice(l, pos);
				while (pos < m_size && at(pos).isspace())
					++pos;
			}

			return res;
		}

		/*
		* @brief 根据pattern切分字符串,数组里的元素浅拷贝
		*
		* @details 可以使用pattern.join(array)来还原字符串
		*
		* @param pattern 切分的模式
		*
		* @param maxsplit 最大切分次数, -1表示无限制
		*
		* @return Array<self> 切分后的字符串数组
		*/
		Array<self> split(const self& pattern, c_size maxsplit = -1) const
		{
			c_size n = count(pattern);

			if (maxsplit != -1 && maxsplit < n)
				n = maxsplit;
			if (n == 0) return { *this };

			Array<self> res(n + 1);
			c_size m_size = size(), p_size = pattern.size();
			c_size l = 0, pos = 0, i = 0;
			while (i <= n)
			{
				l = pos;
				if (i == n)
					pos = m_size;
				else
					pos = index(pattern, pos);

				res[i++] = vslice(l, pos);
				pos += p_size;
			}
			return res;
		}

		constexpr c_size to_int() const
		{
			if (empty()) return 0;

			c_size num = 0;
			bool neg = false;
			auto it = begin(), end_it = end();
			if (*it == '-')
			{
				neg = true;
				++it;
			}
			else if (*it == '+')
				++it;

			while (it != end_it)
			{
				if (it->isdigit())
					num = num * 10 + it->ord() - '0';
				else
					RuntimeError("Invalid character in integer string");
				++it;
			}
			return ifelse(neg, -num, num);
		}

		constexpr double to_double() const
		{
			if (empty()) return 0;

			double num = 0.0;
			bool neg = false;
			bool frac = false;
			double frac_scale = 0.1;

			auto it = begin(), end_it = end();
			if (*it == '-') {
				neg = true;
				++it;
			}
			else if (*it == '+')
				++it;

			while (it != end_it)
			{
				if (!it->isdigit())
				{
					if (!frac && *it == '.')
					{
						frac = true;
						++it;
						continue;
					}
					RuntimeError("Invalid character in double string");
				}

				if (frac)
				{
					num += (it->ord() - '0') * frac_scale;
					frac_scale *= 0.1;
				}
				else
					num = num * 10.0 + (it->ord() - '0');
				++it;
			}

			return neg ? -num : num;
		}

		// 将字符串编码为指定的编码格式
		template<UniCodec C = Codec>
		CString encode() const
		{
			Buffer buffer;
			C{}.encode(data(), size(), buffer);
			return from_buffer(std::move(buffer));
		}

		/*
		* @brief 将字符串编码为指定的编码格式
		* 
		* @param encoding 指定的编码格式
		* utf8, utf16, utf32
		*/
		CString encode(const self& encoding) const
		{
			if (encoding.lower() == from("utf8"))
				return encode<UTF8Codec>();
			else if (encoding.lower() == from("utf16"))
				return encode<UTF16Codec>();
			else if (encoding.lower() == from("utf32"))
				return encode<UTF32Codec>();
			ValueError("Valid encoding is 'utf8', 'utf16', 'utf32'");
			return None;
		}

		// 使用Codec编码字符串
		template<UniCodec C = Codec>
		constexpr static self from(const CString& bytes) { return self(bytes, C{}); }

		// utf8编码字符串
		constexpr static self from_utf8(const CString& bytes) { return from<UTF8Codec>(bytes); }

		// utf16编码字符串
		constexpr static self from_utf16(const CString& bytes) { return from<UTF16Codec>(bytes); }

		// utf32编码字符串
		constexpr static self from_utf32(const CString& bytes) { return from<UTF32Codec>(bytes); }

		// 拼接迭代器对象
		template<IteratableU<self> Obj>
		constexpr static self ajoin(const Obj& elems)
		{
			c_size res_size = 0;
			for (const self& elem : elems)
				res_size += elem.size();

			self res;
			if (res_size > 0)
			{
				AChar* ptr = res.assign_ptr(res_size);
				for (const self& elem : elems)
					ptr = std::copy(elem.begin(), elem.end(), ptr);
			}
			return res;
		}

		cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0; i < m_size && i < o_size; ++i)
				if (at(i) != other.at(i))
					return at(i).ord() - other.at(i).ord();
			return m_size - o_size;
		}

		bool __equals__(const self& other) const
		{
			if (size() != other.size()) return false;
			return std::equal(begin(), end(), other.begin());
		}

		hash_t __hash__() const { return bytes_hash(reinterpret_cast<const char*>(data()), size() * sizeof(AChar)); }

		void __repr__(Buffer& buffer) const { Codec{}.encode(data(), size(), buffer); }

		CString __str__() const { return encode(); }
	private:
		// 获得AChar字符序列的首地址
		constexpr const AChar* data() const { return ifelse(sso(), short_str, long_str); }

		/*
		* @brief 根据length分配内存，返回首地址
		*
		* @details 若length <= SSO_SIZE, 则分配sso优化内存，否则分配堆内存
		*
		* @param length 字符串长度
		*
		* @return AChar* 首地址
		*/
		constexpr AChar* assign_ptr(c_size length)
		{
			AChar* ptr = nullptr;
			if (length <= SSO_SIZE)
			{
				ptr = short_str;
				owner_sso_length_flag = length | SSO_MASK;
			}
			else
			{
				ptr = ayr_alloc<AChar>(length);
				long_str = ptr;
				owner_sso_length_flag = length | OWNER_MASK;
			}

			return ptr;
		}
	};

	namespace literals
	{
		// 字符串字面量，使用utf8编码
		constexpr Atring operator""au8(const char* bytes, size_t size) { return Atring::from_utf8(vstr(bytes, size)); }

		// 字符串字面量，使用utf16编码
		constexpr Atring operator""au16(const char* bytes, size_t size) { return Atring::from_utf16(vstr(bytes, size)); }

		// 字符串字面量，使用utf32编码
		constexpr Atring operator""au32(const char* bytes, size_t size) { return Atring::from_utf32(vstr(bytes, size)); }

		// 字符串字面量，使用默认的编码格式
		constexpr Atring operator""as(const char* bytes, size_t size) { return Atring::from<Codec>(vstr(bytes, size)); }
	}

	using namespace literals;
}
#endif // AYR_BASE_CODEC_ASTRING_HPP