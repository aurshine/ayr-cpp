#ifndef AYR_STRING_HPP
#define AYR_STRING_HPP

#include <utility>

#include "base/AChar.hpp"

namespace ayr
{
	class AtringOrigin : public Object<AtringOrigin>
	{
		using self = AtringOrigin;

		AChar* shared_head_;

		c_size length_;

		Encoding* encoding_;
	public:
		AtringOrigin(Encoding* encoding) : AtringOrigin(nullptr, 0, encoding) {}

		AtringOrigin(AChar* shared_head, c_size length, Encoding* encoding) :
			shared_head_(shared_head), length_(length), encoding_(encoding) {}

		~AtringOrigin() { reset(nullptr, 0); }

		Encoding* const encoding() const { return encoding_; }

		AChar* const shared_head() const { return shared_head_; }

		void reset(AChar* shared_head, c_size length)
		{
			ayr_desloc(shared_head_, length_);
			shared_head_ = shared_head;
			length_ = length;
		}

		void __swap__(self& other)
		{
			swap(shared_head_, other.shared_head_);
			swap(length_, other.length_);
			swap(encoding_, other.encoding_);
		}
	};

	class Atring : public Object<Atring>
	{
		using self = Atring;

		Atring(AChar* achars, c_size length, const std::shared_ptr<AtringOrigin>& origin) :
			achars_(achars), length_(length), origin_(origin) {}

		Atring(size_t n, Encoding* encoding) :
			Atring(ayr_alloc<AChar>(n), n, std::make_shared<AtringOrigin>(encoding))
		{
			for (int i = 0; i < n; ++i)
				ayr_construct(achars_ + i);
			origin_->reset(achars_, n);
		}
	public:
		Atring(Encoding* encoding = UTF8) : Atring(0, encoding) {}

		Atring(const char* str, c_size len = -1, Encoding* encoding = UTF8) : Atring(0, encoding)
		{
			len = ifelse(len > 0, len, std::strlen(str));
			auto cps_info = get_cps(str, len, origin_->encoding()).move_array().separate();

			achars_ = cps_info.first;
			length_ = cps_info.second;
			origin_->reset(achars_, length_);
		}

		Atring(const CString& other, Encoding* encoding = UTF8) : Atring(other.data(), other.size(), encoding) {}

		Atring(const self& other) : Atring(other.achars_, other.length_, other.origin_) {}

		self& operator= (const self& other)
		{
			if (this == &other) return *this;
			achars_ = other.achars_;
			length_ = other.length_;
			origin_ = other.origin_;
			return *this;
		}

		self operator+ (const self& other) const
		{
			self result(size() + other.size(), encoding());
			size_t m_size = size(), o_size = other.size();
			for (size_t i = 0; i < m_size; ++i)
				result.achars_[i] = atchar(i);

			for (size_t i = 0; i < o_size; ++i)
				result.achars_[m_size + i] = other.atchar(i);
			return result;
		}

		self& operator+= (const self& other)
		{
			self result = *this + other;
			*this = *this + other;
			return *this;
		}

		self operator* (size_t n) const
		{
			self result(size() * n, encoding());

			size_t pos = 0, m_size = size();
			while (n--)
				for (size_t i = 0; i < m_size; ++i)
					result.achars_[pos++] = atchar(i);

			return result;
		}

		self at(c_size index) const { return slice(index, index + 1); }

		const AChar& atchar(c_size index) const { return achars_[index]; }

		self operator[] (c_size index) const { return at(neg_index(index, size())); }

		c_size size() const { return length_; }

		Encoding* const encoding() const { return origin_->encoding(); }

		// 字符串的字节长度
		c_size byte_size() const
		{
			c_size size_ = 0;
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				size_ += atchar(i).size();
			return size_;
		}

		CString __str__() const
		{
			CString result(byte_size());

			c_size pos = 0;
			for (int i = 0, m_size = size(); i < m_size; ++i)
			{
				c_size c_size_ = atchar(i).size();
				std::memcpy(result.data() + pos, atchar(i).data(), c_size_);
				pos += c_size_;
			}
			return result;
		}

		hash_t __hash__() const { return __str__().__hash__(); }

		cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0, j = 0; i < m_size && j < o_size; ++i, ++j)
			{
				if (atchar(i) < other.atchar(j))
					return -1;
				else if (atchar(i) > other.atchar(j))
					return 1;
			}

			return m_size - o_size;
		}

		bool __equals__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size) return false;
			for (c_size i = 0; i < m_size; ++i)
				if (atchar(i) != other.atchar(i))
					return false;
			return true;
		}

		void __swap__(self& other)
		{
			swap(achars_, other.achars_);
			swap(length_, other.length_);
			swap(origin_, other.origin_);
		}

		c_size find(const self& pattern, c_size pos = 0) const
		{
			c_size m_size = size(), pattern_size = pattern.size();
			for (c_size i = pos; i + pattern_size <= m_size; ++i)
				if (slice(i, i + pattern_size) == pattern)
					return i;

			return -1;
		}

		Array<c_size> find_all(const self& pattern) const
		{
			DynArray<c_size> result;

			c_size pos = 0;
			while (true)
			{
				pos = find(pattern, pos);
				if (pos != -1)
					result.append(pos);
				else
					break;
				pos += pattern.size();
			}
			return result.to_array();
		}

		// 切片[start, end)，内容浅拷贝
		self slice(c_size start, c_size end) const
		{
			c_size size_ = size();
			start = neg_index(start, size_);
			end = neg_index(end, size_);
			return self(achars_ + start, end - start, origin_);
		}

		// 切片[start, size())，内容浅拷贝
		self slice(c_size start) const { return slice(start, size()); }

		// 判断是否以prefix开头
		bool startswith(const self& prefix) const
		{
			if (prefix.size() > size())
				return false;

			return slice(0, prefix.size()) == prefix;
		}

		// 判断是否以suffix结尾
		bool endswith(const self& suffix) const
		{
			if (suffix.size() > size())
				return false;

			return slice(size() - suffix.size()) == suffix;
		}

		// 判断是否为空字符串
		bool isspace() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).isspace())
					return false;
			return true;
		}

		// 判断是否为数字字符串
		bool isdigit() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).isdigit())
					return false;
			return true;
		}

		// 判断是否为字母字符串
		bool isalpha() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).isalpha())
					return false;
			return true;
		}

		// 判断是否为大写字符串
		bool isupper() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).isupper())
					return false;
			return true;
		}

		// 判断是否为小写字符串
		bool islower() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).islower())
					return false;
			return true;
		}

		// 判断是否为ASCII字符串
		bool isasciii() const
		{
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
				if (!atchar(i).isasciii())
					return false;
			return true;
		}

		// 返回新的大写字符串
		self upper() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.achars_[i] = result.atchar(i).upper();
			return result;
		}

		// 返回新的小写字符串
		self lower() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.achars_[i] = result.atchar(i).lower();
			return result;
		}

		// 去除两端空白，浅拷贝
		self strip() const
		{
			auto [l, r] = _get_strip_index();
			return slice(l, r);
		}

		// 去除两端pattern，浅拷贝
		self strip(const self& pattern) const
		{
			auto [l, r] = _get_strip_index(pattern);
			return slice(l, r);
		}

		// 去除左侧空白，浅拷贝
		self lstrip() const { return slice(_get_lstrip_index()); }

		// 去除左侧pattern，浅拷贝
		self lstrip(const self& pattern) const { return slice(_get_lstrip_index(pattern)); }

		// 去除右侧空白，浅拷贝
		self rstrip() const { return slice(_get_rstrip_index()); }

		// 去除右侧pattern，浅拷贝
		self rstrip(const self& pattern) const { return slice(_get_rstrip_index(pattern)); }

		// 连接字符串序列，返回新的字符串
		template<IteratableU<self> Obj>
		self join(const Obj& elems) const
		{
			c_size new_length = 0, pos = 0, m_size = size();
			for (const self& elem : elems)
				new_length += elem.size() + m_size;
			new_length = std::max<c_size>(0, new_length - m_size);

			auto put_back = [&pos](self& str, const self& other) {
				for (auto&& c : std::ranges::subrange(other.achars_, other.achars_ + other.size()))
					str.achars_[pos++] = c;
				};

			self result(new_length, encoding());
			for (const self& elem : elems)
			{
				static bool started = false;
				if (started) put_back(result, *this);
				put_back(result, elem);
				started = true;
			}

			return result;
		}

		// 替换old_为new_，返回新的字符串
		self replace(const self& old_, const self& new_) const
		{
			DynArray<c_size> indices;
			c_size pos = 0;
			while (pos = find(old_, pos), pos != -1)
			{
				indices.append(pos);
				pos += old_.size();
			}

			self result(size() + indices.size() * (new_.size() - old_.size()), encoding());
			for (c_size i = 0, j = 0, k = 0; i < size(); ++i)
			{
				if (j < indices.size() && i == indices[j])
				{
					for (auto&& c : std::ranges::subrange(new_.achars_, new_.achars_ + new_.size()))
						result.achars_[k++] = c;
					i += old_.size() - 1;
					j++;
				}
				else
					result.achars_[k++] = atchar(i);
			}

			return result;
		}

		// 根据空白符切分字符串,数组里的元素浅拷贝
		Array<self> split() const
		{
			c_size l = 0, r = 0, size_ = size();
			DynArray<self> result;
			while (r < size_)
			{
				if (atchar(r).isspace())
				{
					if (r > l) result.append(slice(l, r));
					l = r + 1;
				}
				++r;
			}

			if (r > l) result.append(slice(l));
			return result.to_array();
		}

		// 根据pattern切分字符串,数组里的元素浅拷贝
		Array<self> split(const self& pattern) const
		{
			c_size l = 0, r = 0, size_ = size(), p_size = pattern.size();
			DynArray<self> result;

			while (r + p_size <= size_)
			{
				if (pattern == slice(r, r + p_size))
				{
					if (r > l) result.append(slice(l, r));
					r += p_size;
					l = r;
				}
				else
					++r;
			}

			if (r > l) result.append(slice(l));
			return result.to_array();
		}

		self match(const self& lmatch, const self& rmatch) const
		{
			c_size l = 0, size_ = size(), l_size = lmatch.size(), r_size = rmatch.size();

			while (l + l_size <= size_ && slice(l, l + l_size) != lmatch) ++l;
			if (l + l_size > size_) return "";

			c_size match_cnt = 1;
			for (c_size r = l + l_size; r + l_size <= size_ && r + r_size <= size_; ++r)
			{
				if (slice(r, r + r_size) == rmatch)
					--match_cnt;
				else if (slice(r, r + l_size) == lmatch)
					++match_cnt;

				if (match_cnt == 0) return slice(l, r + r_size);
			}

			ValueError("Unmatched parentheses, too many left parentheses");
			return None<self>;
		}

		class AtringIterator : public IteratorInfo<AtringIterator, Atring, std::forward_iterator_tag, Atring>
		{
		public:
			using self = AtringIterator;

			using ItInfo = IteratorInfo<self, Atring, std::forward_iterator_tag, Atring>;

			AtringIterator() : src_(), pos_(0) {}

			AtringIterator(const Atring* src, c_size pos) : src_(src), pos_(pos) {}

			AtringIterator(const self& other) : src_(other.src_), pos_(other.pos_) {}

			self& operator=(const self& other)
			{
				src_ = other.src_;
				pos_ = other.pos_;
				return *this;
			}

			typename ItInfo::value_type operator*() const { return src_->slice(pos_, pos_ + 1); }

			self& operator++() { ++pos_; return *this; }

			self operator++(int) { return self(src_, pos_++); }

			self& operator--() { --pos_; return *this; }

			self operator--(int) { return self(src_, pos_--); }

			self operator+(c_size n) const { return self(src_, pos_ + n); }

			self& operator+= (c_size n) { pos_ += n; return *this; }

			self operator-(c_size n) const { return self(src_, pos_ - n); }

			self& operator-= (c_size n) { pos_ -= n; return *this; }

			c_size operator-(const self& other) const { return pos_ - other.pos_; }

			bool __equals__(const self& other) const { return src_ == other.src_ && pos_ == other.pos_; }
		private:
			const ItInfo::container_type* src_;

			c_size pos_;
		};

		using Iterator = AtringIterator;

		using ConstIterator = AtringIterator;

		Iterator begin() const { return Iterator(this, 0); }

		Iterator end() const { return Iterator(this, size()); }

	private:
		std::pair<c_size, c_size> _get_strip_index() const
		{
			c_size l = 0, r = size();
			while (l < r && atchar(l).isspace()) ++l;
			while (l < r && atchar(r - 1).isspace()) --r;
			return { l, r };
		}

		std::pair<c_size, c_size> _get_strip_index(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			while (l + p_size <= r && slice(l, l + p_size) == pattern) l += p_size;
			while (l + p_size <= r && slice(r - p_size, r) == pattern) r -= p_size;
			return { l, r };
		}

		c_size _get_lstrip_index() const
		{
			c_size l = 0, r = size();
			while (l < r && atchar(l).isspace()) ++l;
			return l;
		}

		c_size _get_lstrip_index(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			while (l + p_size <= r && slice(l, l + p_size) == pattern) l += p_size;
			return l;
		}

		c_size _get_rstrip_index() const
		{
			c_size l = 0, r = size();
			while (l < r && atchar(r - 1).isspace()) --r;
			return r;
		}

		c_size _get_rstrip_index(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			while (l + p_size <= r && slice(r - p_size, r) == pattern) r -= p_size;
			return r;
		}
	private:
		AChar* achars_;

		c_size length_;

		std::shared_ptr<AtringOrigin> origin_;
	};

	inline Atring operator ""as(const char* str, size_t len) { return Atring(str, len); }
}
#endif