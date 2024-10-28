#ifndef AYR_STRING_HPP
#define AYR_STRING_HPP

#include <ayr/detail/CodePoint.hpp>

namespace ayr
{
	class Atring : public Sequence<CodePoint>
	{
		using self = Atring;

		using super = Sequence<CodePoint>;

		Atring(CodePoint* codepoints, c_size length, std::shared_ptr<CodePoint[]> shared_head, Encoding* encoding) noexcept :
			codepoints_(codepoints), length_(length), shared_head_(shared_head), encoding_(encoding) {}

		Atring(size_t n, Encoding* encoding) :
			Atring(nullptr, n, std::make_shared<CodePoint[]>(n), encoding)
		{
			codepoints_ = shared_head_.get();
		}
	public:
		Atring(const CString& encoding = UTF8) : Atring(nullptr, 0, nullptr, encoding_map(encoding)) {}

		Atring(const char* str, c_size len = -1, const CString& encoding = UTF8) : Atring(nullptr, 0, nullptr, encoding_map(encoding))
		{
			len = ifelse(len > 0, len, std::strlen(str));
			auto cps_info = get_cps(str, len, encoding_).move_array().separate();

			codepoints_ = cps_info.first;
			length_ = cps_info.second;
			shared_head_.reset(codepoints_);
		}

		Atring(const CString& other, const CString& encoding = UTF8) : Atring(other.data(), other.size(), encoding) {}

		Atring(const self& other) : Atring(other.size(), other.encoding_)
		{
			for (int i = 0, size_ = size(); i < size_; ++i)
				at(i) = other.at(i);
		}

		Atring(self&& other) noexcept : Atring(other.codepoints_, other.length_, other.shared_head_, other.encoding_) {}

		~Atring() {};

		self& operator= (const self& other)
		{
			if (this == &other)
				return *this;

			length_ = other.length_;
			shared_head_ = std::make_shared<CodePoint[]>(length_);
			codepoints_ = shared_head_.get();
			for (int i = 0; i < length_; ++i)
				at(i) = other.at(i);
			return *this;
		}

		self& operator= (self&& other) noexcept
		{
			if (this == &other) return *this;

			codepoints_ = other.codepoints_;
			length_ = other.length_;
			shared_head_ = other.shared_head_;
			return *this;
		}

		self operator+ (const self& other) const
		{
			self result(size() + other.size(), encoding_);
			size_t m_size = size(), o_size = other.size();
			for (size_t i = 0; i < m_size; ++i)
				result.at(i) = at(i);

			for (size_t i = 0; i < o_size; ++i)
				result.at(m_size + i) = other.at(i);
			return result;
		}

		self& operator+= (const self& othre)
		{
			self result = *this + othre;
			*this = std::move(result);
			return *this;
		}

		self operator* (size_t n)
		{
			self result(size() * n, encoding_);

			size_t pos = 0, m_size = size();
			while (n--)
				for (size_t i = 0; i < m_size; ++i)
					result.at(pos++) = at(i);

			return result;
		}

		CodePoint& at(c_size index) { return codepoints_[index]; }

		const CodePoint& at(c_size index) const { return codepoints_[index]; }

		c_size size() const override { return length_; }

		// 字符串的字节长度
		c_size byte_size() const
		{
			c_size size_ = 0;
			for (auto&& c : *this)
				size_ += c.size();
			return size_;
		}

		CString __str__() const
		{
			CString result(byte_size());

			c_size pos = 0;
			for (auto&& c : *this)
			{
				c_size c_size_ = c.size();
				std::memcpy(result.data() + pos, c.data(), c_size_);
				pos += c_size_;
			}
			return result;
		}

		hash_t __hash__() const { return __str__().__hash__(); }

		c_size find(CodePoint c) const { return super::find(c); }

		c_size find(const self& pattern, c_size pos = 0) const
		{
			c_size m_size = size(), pattern_size = pattern.size();
			for (c_size i = pos; i + pattern_size < m_size; ++i)
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

		self slice(c_size start, c_size end)
		{
			start = neg_index(start, size());
			end = neg_index(end, size());
			self result;
			result.codepoints_ = codepoints_ + start;
			result.length_ = end - start;
			result.shared_head_ = shared_head_;
			return result;
		}

		const self slice(c_size start, c_size end) const
		{
			start = neg_index(start, size());
			end = neg_index(end, size());
			self result;
			result.codepoints_ = codepoints_ + start;
			result.length_ = end - start;
			result.shared_head_ = shared_head_;
			return result;
		}

		self slice(c_size start) { return slice(start, size()); }

		const self slice(c_size start) const { return slice(start, size()); }

		bool startswith(const self& prefix) const
		{
			if (prefix.size() > size())
				return false;

			return slice(0, prefix.size()) == prefix;
		}

		bool endswith(const self& suffix) const
		{
			if (suffix.size() > size())
				return false;

			return slice(size() - suffix.size()) == suffix;
		}

		self upper() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.at(i) = result.at(i).upper();
			return result;
		}

		self lower() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.at(i) = result.at(i).lower();
			return result;
		}

		self strip()
		{
			auto [l, r] = _get_strip_index();
			return slice(l, r);
		}

		const self strip() const
		{
			auto [l, r] = _get_strip_index();
			return slice(l, r);
		}

		self strip(const self& pattern)
		{
			auto [l, r] = _get_strip_index(pattern);
			return slice(l, r);
		}

		const self strip(const self& pattern) const
		{
			auto [l, r] = _get_strip_index(pattern);
			return slice(l, r);
		}

		self lstrip() { return slice(_get_lstrip_index()); }

		const self lstrip() const { return slice(_get_lstrip_index()); }

		self lstrip(const self& pattern) { return slice(_get_lstrip_index(pattern)); }

		const self lstrip(const self& pattern) const { return slice(_get_lstrip_index(pattern)); }

		self rstrip() { return slice(_get_rstrip_index()); }

		const self rstrip() const { return slice(_get_rstrip_index()); }

		self rstrip(const self& pattern) { return slice(_get_rstrip_index(pattern)); }

		const self rstrip(const self& pattern) const { return slice(_get_rstrip_index(pattern)); }

		template<Iteratable I>
		self join(const I& iter) const
		{
			c_size new_length = 0, pos = 0, m_size = size();
			for (auto&& elem : iter)
				new_length += elem.size() + m_size;
			new_length = std::max(0ll, new_length - m_size);

			self result(new_length);
			for (auto&& elem : iter)
			{
				for (c_size i = 0, e_size; i < e_size; ++i)
					result.at(pos++) = elem.at(i);

				for (c_size i = 0; i < m_size; ++i)
					result.at(pos++) = at(i);
			}

			return result;
		}

		self replace(const self& old_, const self& new_) const
		{
			DynArray<c_size> indices;
			c_size pos = 0;
			while (pos = find(old_, pos), pos != -1)
			{
				indices.append(pos);
				pos += old_.size();
			}

			self result(size() + indices.size() * (new_.size() - old_.size()), encoding_);
			for (c_size i = 0, j = 0, k = 0; i < size(); ++i)
			{
				if (j < indices.size() && i == indices[j])
				{
					for (auto&& c : new_)
						result.codepoints_[k++] = c;
					i += old_.size() - 1;
					j++;
				}
				else
					result.codepoints_[k++] = at(i);
			}

			return result;
		}

		self split() const
		{

		}

		self split(const self& pattern) const
		{

		}

		self match(CodePoint lmatch, CodePoint rmatch) const
		{
			c_size l = find(lmatch);
			if (l == -1)
				return "";

			c_size match_cnt = 0;
			for (c_size i = l; i < size(); ++i)
			{
				if (at(i) == lmatch)
					++match_cnt;
				else if (at(i) == rmatch)
					--match_cnt;

				if (match_cnt == 0)
					return slice(l, i + 1);
			}

			ValueError(std::format("Unmatched parentheses, too many left parentheses"));
		}
	private:
		std::pair<c_size, c_size> _get_strip_index() const
		{
			c_size l = 0, r = size();
			while (l < r && at(l).isspace()) ++l;
			while (l < r && at(r - 1).isspace()) --r;
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
			while (l < r && at(l).isspace()) ++l;
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
			while (l < r && at(r - 1).isspace()) --r;
			return r;
		}

		c_size _get_rstrip_index(const self& pattern) const
		{
			c_size l = 0, r = size(), p_size = pattern.size();
			while (l + p_size <= r && slice(r - p_size, r) == pattern) r -= p_size;
			return r;
		}
	private:
		CodePoint* codepoints_;

		c_size length_;

		std::shared_ptr<CodePoint[]> shared_head_;

		Encoding* encoding_;
	};



	inline Atring operator ""as(const char* str, size_t len) { return Atring(str, len); }
}
#endif