#ifndef AYR_STRING_HPP
#define AYR_STRING_HPP

#include <utility>

#include "Array.hpp"
#include "DynArray.hpp"
#include "base/AChar.hpp"
#include "base/View.hpp"


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
			if (encoding() != other.encoding())
				RuntimeError("Cannot concatenate strings with different encodings");

			self result(size() + other.size(), encoding());
			size_t m_size = size(), o_size = other.size();
			for (size_t i = 0; i < m_size; ++i)
				result.achars_[i] = atchar(i);

			for (size_t i = 0; i < o_size; ++i)
				result.achars_[m_size + i] = other.atchar(i);
			return result;
		}

		self& operator+= (const self& other) { return *this = *this + other; }

		self operator* (size_t n) const { return ajoin(Array<ViewOF<self>>(n, view_of(*this))); }

		const AChar& atchar(c_size index) const { return achars_[index]; }

		std::ranges::subrange<const AChar*> chars() const { return std::ranges::subrange<const AChar*>(achars_, achars_ + length_); }

		self at(c_size index) const { return slice(index, index + 1); }

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
			DynArray<CString> results;
			for (const AChar& c : chars())
				results.append(cstr(c));
			return CString::cjoin(results);
		}

		void __repr__(Buffer& buffer) const
		{
			for (const AChar& c : chars())
				buffer << c;
		}

		hash_t __hash__() const { return __str__().__hash__(); }

		cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0, j = 0; i < m_size && j < o_size; ++i, ++j)
			{
				cmp_t cmp_ = atchar(i).__cmp__(other.atchar(j));
				if (cmp_ != 0) return cmp_;
			}

			return m_size - o_size;
		}

		cmp_t __cmp__(char c) const
		{
			if (size() == 0) return -1;
			cmp_t cmp_ = atchar(0).__cmp__(c);
			if (cmp_ == 0) return size() != 1;
			return cmp_;
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

		Array<c_size> find_all(const self& pattern, c_size count = -1) const
		{
			DynArray<c_size> result;

			c_size pos = 0;
			while (count != 0)
			{
				pos = find(pattern, pos);
				if (pos == -1) break;
				result.append(pos);
				pos += pattern.size();
				--count;
			}
			return result.to_array();
		}

		c_size count(const self& pattern) const { return find_all(pattern).size(); }

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
			if (size() == 0) return ajoin(elems);

			c_size new_length = 0, pos = 0, m_size = size();
			for (const self& elem : elems)
			{
				if (encoding() != elem.encoding())
					RuntimeError("Cannot concatenate strings with different encodings");
				new_length += elem.size() + m_size;
			}

			new_length = std::max<c_size>(0, new_length - m_size);

			self result(new_length, encoding());
			auto put_back = [&pos, &result](const self& other) {
				for (const AChar& c : std::ranges::subrange(other.achars_, other.achars_ + other.size()))
					result.achars_[pos++] = c;
				};

			bool started = false;
			for (const self& elem : elems)
			{
				if (started) put_back(*this);
				put_back(elem);
				started = true;
			}

			return result;
		}

		template<IteratableU<Atring> Obj>
		static Atring ajoin(const Obj& elems)
		{
			c_size length = 0;
			Encoding* encoding = nullptr;

			for (const Atring& str : elems)
			{
				length += str.size();
				if (encoding == nullptr)
					encoding = str.encoding();
				else if (encoding != str.encoding())
					RuntimeError("Strings have different encodings");
			}

			Atring result(length, encoding);
			c_size pos = 0;
			for (const Atring& str : elems)
				for (c_size i = 0, n = str.size(); i < n; ++i)
					result.achars_[pos++] = str.achars_[i];

			return result;
		}

		// 替换old_为new_，返回新的字符串
		self replace(const self& old_, const self& new_, c_size maxreplace = -1) const
		{
			Array<c_size> indices = find_all(old_, maxreplace);

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
		Array<self> split(c_size maxsplit = -1) const
		{
			c_size l = 0, r = 0, size_ = size();
			DynArray<self> result;
			while (r < size_ && maxsplit != 0)
			{
				if (atchar(r).isspace())
				{
					if (r > l) result.append(slice(l, r));
					l = r + 1;
					--maxsplit;
				}
				++r;
			}

			if (r > l) result.append(slice(l));
			return result.to_array();
		}

		// 根据pattern切分字符串,数组里的元素浅拷贝
		Array<self> split(const self& pattern, c_size maxsplit = -1) const
		{
			c_size size_ = size(), p_size = pattern.size();
			Array<c_size> indices = find_all(pattern, maxsplit);
			c_size res_size = indices.size() - 1;

			if (indices.front() != 0) res_size += 1;
			if (indices.back() + p_size != size_) res_size += 1;
			Array<self> result(res_size, encoding());

			c_size k = 0;
			if (indices.front() != 0) result[k++] = slice(0, indices.front());
			for (c_size i = 1; i < indices.size(); ++i)
				result[k++] = slice(indices[i - 1] + p_size, indices[i]);
			if (indices.back() + p_size != size_) result[k++] = slice(indices.back() + p_size);
			return result;
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
			return None;
		}

		c_size to_int() const
		{
			if (size() == 0) return 0;
			c_size res = 0;
			bool neg = atchar(0) == '-';
			self num_body = ifelse(neg, slice(1), *this);

			if (!num_body.isdigit())
				RuntimeError("string is not a valid integer");

			for (c_size i = 0, n = num_body.size(); i < n; ++i)
				res = res * 10 + num_body.atchar(i).ord(encoding()) - '0';

			return ifelse(neg, -res, res);
		}

		double to_double() const
		{
			Array<c_size> dot_indices = find_all(".");

			if (dot_indices.size() == 0)
				return to_int();

			if (dot_indices.size() == 1)
			{
				self int_part = slice(0, dot_indices[0]);
				self float_part = slice(dot_indices[0] + 1);

				c_size int_val = int_part.to_int();
				if (!float_part.isdigit())
					RuntimeError("string is not a valid float");
				double float_val = 0;
				for (c_size i = float_part.size() - 1; i >= 0; --i)
					float_val = (float_val + float_part.atchar(i).ord(encoding()) - '0') / 10.0;
				return ifelse(int_val >= 0, int_val + float_val, int_val - float_val);
			}
			else
				RuntimeError("string is not a valid float");

			return 0;
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