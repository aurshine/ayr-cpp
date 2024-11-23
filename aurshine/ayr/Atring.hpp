#ifndef AYR_STRING_HPP
#define AYR_STRING_HPP

#include <ayr/base/CodePoint.hpp>
#include <ayr/base/NoCopy.hpp>

namespace ayr
{
	class AtringManager : public Object<AtringManager>, public NoCopy
	{
		using self = AtringManager;

		CodePoint* shared_head_;

		c_size length_;

		Encoding* encoding_;
	public:
		AtringManager(CodePoint* shared_head, c_size length, Encoding* encoding) :
			shared_head_(shared_head), length_(length), encoding_(encoding) {}

		~AtringManager()
		{
			ayr_destroy(shared_head_, length_);
			ayr_delloc(shared_head_);
			shared_head_ = nullptr;
			length_ = 0;
		}

		Encoding* const encoding() const { return encoding_; }

		CodePoint* const shared_head() const { return shared_head_; }

		void reset(CodePoint* shared_head, c_size length)
		{
			ayr_destroy(shared_head_, length_);
			ayr_delloc(shared_head_);
			shared_head_ = shared_head;
			length_ = length;
		}
	};

	class Atring : public Sequence<Atring, CodePoint>
	{
		using self = Atring;

		using super = Sequence<self, CodePoint>;

		Atring(CodePoint* codepoints, c_size length, std::shared_ptr<AtringManager> manager) :
			codepoints_(codepoints), length_(length), manager_(manager) {}

		Atring(size_t n, Encoding* encoding) :
			Atring(nullptr, n, std::make_shared<AtringManager>(ayr_alloc<CodePoint>(n), n, encoding))
		{
			for (int i = 0; i < n; ++i)
				ayr_construct(manager_->shared_head() + i);
			codepoints_ = manager_->shared_head();
		}
	public:
		Atring(const CString& encoding = UTF8) : Atring(0, encoding_map(encoding)) {}

		Atring(const char* str, c_size len = -1, const CString& encoding = UTF8) : Atring(0, encoding_map(encoding))
		{
			len = ifelse(len > 0, len, std::strlen(str));
			auto cps_info = get_cps(str, len, manager_->encoding()).move_array().separate();

			codepoints_ = cps_info.first;
			length_ = cps_info.second;
			manager_->reset(codepoints_, length_);
		}

		Atring(const CString& other, const CString& encoding = UTF8) : Atring(other.data(), other.size(), encoding) {}

		Atring(const self& other) : Atring(other.size(), other.encoding())
		{
			for (int i = 0, size_ = size(); i < size_; ++i)
				at(i) = other.at(i);
		}

		Atring(self&& other) noexcept : Atring(other.codepoints_, other.length_, other.manager_) {}

		~Atring() {};

		self& operator= (const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(&manager_);

			return *ayr_construct(this, other);
		}

		self& operator= (self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(&manager_);

			return *ayr_construct(this, std::move(other));
		}

		self operator+ (const self& other) const
		{
			self result(size() + other.size(), encoding());
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
			self result(size() * n, encoding());

			size_t pos = 0, m_size = size();
			while (n--)
				for (size_t i = 0; i < m_size; ++i)
					result.at(pos++) = at(i);

			return result;
		}

		CodePoint& at(c_size index) { return codepoints_[index]; }

		const CodePoint& at(c_size index) const { return codepoints_[index]; }

		c_size size() const { return length_; }

		Encoding* const encoding() const { return manager_->encoding(); }

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
			c_size size_ = size();
			start = neg_index(start, size_);
			end = neg_index(end, size_);
			return self(codepoints_ + start, end - start, manager_);
		}

		const self slice(c_size start, c_size end) const
		{
			c_size size_ = size();
			start = neg_index(start, size_);
			end = neg_index(end, size_);
			return self(codepoints_ + start, end - start, manager_);
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
			new_length = std::max<c_size>(0ll, new_length - m_size);

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

			self result(size() + indices.size() * (new_.size() - old_.size()), encoding());
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

			c_size match_cnt = 1;
			for (c_size r = l + 1, size_ = size(); r < size_; ++r)
			{
				if (at(r) == rmatch)
					--match_cnt;
				else if (at(r) == lmatch)
					++match_cnt;

				if (match_cnt == 0)
					return slice(l, r + 1);
			}

			ValueError("Unmatched parentheses, too many left parentheses");
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

		std::shared_ptr<AtringManager> manager_;
	};



	inline Atring operator ""as(const char* str, size_t len) { return Atring(str, len); }
}
#endif