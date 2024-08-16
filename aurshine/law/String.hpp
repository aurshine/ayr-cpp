#ifndef AYR_LAW_STRING_HPP
#define AYR_LAW_STRING_HPP


#include <law/printer.hpp>
#include <law/Dynarray.hpp>
#include <law/ayr_memory.hpp>


namespace ayr
{
	template<Char CharT>
	class String : public Sequence<CharT>
	{
		using self = String<CharT>;

		using super = Sequence<CharT>;

		CharT* cstr_;

		c_size length_;

		std::shared_ptr<CharT> shared_head_;

		int* ref_count_;

		CharT* head_;

		String(size_t length, CharT c) : cstr_(nullptr), length_(length), ref_count_(nullptr), head_(nullptr)
		{
			head_ = cstr_ = ayr_alloc(CharT, length + 1);

			ref_count_ = ayr_alloc(int, 1);

			for (size_t i = 0; i < length_; ++i)
				cstr_[i] = c;
			cstr_[length_] = '\0';
			*ref_count_ = 1;
		}
	public:
		String() : String("", 0) {}

		String(self&& other) : String(other) {}

		String(const RawString<CharT>& other) : String(other.str) {}

		String(const self& other) : String(other.cstr_, other.length_) {}

		String(self& other) : cstr_(other.cstr_), length_(other.length_), ref_count_(other.ref_count_), head_(other.head_) { ++ (*other.ref_count_); }

		String(const CharT* str, c_size len = -1) : cstr_(nullptr), length_(0), ref_count_(nullptr)
		{
			length_ = ifelse(len >= 0, len, std::strlen(str));

			head_ = cstr_ = ayr_alloc(CharT, length_ + 1);
			ref_count_ = ayr_alloc(int, 1);
			std::memcpy(cstr_, str, length_ * sizeof(CharT));
			cstr_[length_] = '\0';
			*ref_count_ = 1;
		}

		~String() { release(); }

		self& operator= (self& other)
		{
			if (this == &other)
				return *this;

			release();
			ayr_construct(self, this, other);
			return *this;
		}

		self& operator= (self&& other) { return operator= (other); }

		self& operator= (const self& other)
		{
			if (this == &other)
				return *this;

			release();
			ayr_construct(self, this, other);
			return *this;
		}

		self operator+ (const self& other) const
		{
			self result(size() + other.size(), '\0');
			size_t self_size = size(), other_size = other.size();
			for (size_t i = 0; i < self_size; ++i)
				result[i] = __at__(i);

			for (size_t i = 0; i < other_size; ++i)
				result[self_size + i] = other[i];
			return result;
		}

		self operator+= (const self& othre)
		{
			self result = *this + othre;
			release();
			*this = std::move(result);
			return *this;
		}

		self operator* (size_t n)
		{
			self result(size() * n, '\0');

			size_t pos = 0, self_size = size();
			while (n--)
				for (size_t i = 0; i < self_size; ++i)
					result[pos++] = __at__(i);

			return result;
		}

		const CharT& operator[] (c_size index) const { return __at__(index); }

		const CharT& operator[] (c_size index) { return __at__(index); }

		CharT& __at__(c_size index) { return cstr_[neg_index(index, size())]; }

		const CharT& __at__(c_size index) const { return cstr_[neg_index(index, size())]; }

		c_size size() const override { return length_; }

		hash_t __hash__() const { return bytes_hash(reinterpret_cast<const char*>(cstr_), length_ * sizeof(CharT)); }

		CString __str__() const { return CString(cstr_, length_); }

		c_size find(CharT c, c_size pos) const
		{
			for (c_size i = pos; i < length_; ++i)
				if (cstr_[i] == c)
					return i;

			return -1;
		}

		c_size find(const self& pattern, c_size pos = 0) const
		{
			c_size self_size = size(), pattern_size = pattern.size();
			for (c_size i = pos; i < self_size - pattern_size; ++i)
			{
				bool flag = true;
				for (c_size j = 0; flag && j < pattern_size; ++j)
					flag &= (__at__(i + j) != pattern.__at__(j));

				if (flag) return i;
			}

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

		self& slice_(c_size start, c_size end)
		{
			start = neg_index(start, size());
			end = neg_index(end, size());

			cstr_ += start;
			length_ = end - start;
			return *this;
		}

		self& slice_(c_size start) { return slice_(start, size()); }

		self slice(c_size start, c_size end)
		{
			self sub = *this;
			return sub.slice_(start, end);
		}

		self slice(c_size start, c_size end) const
		{
			self sub = *this;
			return sub.slice_(start, end);
		}

		self slice(c_size start) { return slice(start, size()); }

		self slice(c_size start) const { return slice(start, size()); }

		bool startswith(const self& prefix) const
		{
			if (prefix.size() > size())
				return false;

			for (c_size i = 0; i < prefix.size(); ++i)
				if (prefix.__at__(i) != __at__(i))
					return false;

			return true;
		}

		bool endswith(const self& suffix) const
		{
			if (suffix.size() > size())
				return false;

			c_size suffix_size = suffix.size();
			for (c_size i = 0; i < suffix_size; ++i)
				if (suffix.__at__(i) != __at__(size() - suffix_size + i))
					return false;

			return true;
		}

		self upper() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.__at__(i) = std::toupper(result.__at__(i));
			return result;
		}

		self lower() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.__at__(i) = std::tolower(result.__at__(i));
			return result;
		}

		self& strip_()
		{
			c_size l = 0, r = size();
			while (l < r && std::isspace(__at__(l))) ++l;
			while (l < r && std::isspace(__at__(r - 1))) --r;
			return slice_(l, r);
		}

		self strip() const
		{
			self result = *this;
			return result.strip_();
		}

		self& strip_(const self& pattern)
		{
			c_size l = 0, r = size(), pattern_size = pattern.size();
			while (l < r && startswith(pattern))
			{
				l += pattern_size;
				slice_(l, r);
			}

			while (l < r && endswith(pattern))
			{
				r -= pattern_size;
				slice_(l, r);
			}

			return *this;
		}

		self strip(const self& pattern) const
		{
			self result = *this;
			return result.strip_(pattern);
		}

		self& lstrip_()
		{
			c_size l = 0;
			while (l < size() && std::isspace(__at__(l))) ++l;
			return slice_(l);
		}

		self lstrip() const
		{
			self result = *this;
			return result.lstrip_();
		}

		self& lstrip_(const self& pattern)
		{
			c_size l = 0, pattern_size = pattern.size();
			while (l < size() && startswith(pattern))
			{
				l += pattern_size;
				slice_(l);
			}
			return *this;
		}

		self lstrip(const self& pattern) const
		{
			self result = *this;
			return result.lstrip_(pattern);
		}

		self& rstrip_()
		{
			c_size r = size();
			while (r > 0 && std::isspace(__at__(r - 1))) --r;
			return slice_(0, r);
		}

		self rstrip() const
		{
			self result = *this;
			return result.rstrip_();
		}

		self& rstrip_(const self& pattern)
		{
			c_size r = size(), pattern_size = pattern.size();
			while (r > 0 && endswith(pattern))
			{
				r -= pattern_size;
				slice_(0, r);
			}
			return *this;
		}

		self rstrip(const self& pattern) const
		{
			self result = *this;
			return result.rstrip_(pattern);
		}

		template<typename I>
		self join(const I& iter) const
		{
			c_size new_length = 0, pos = 0;
			for (auto&& elem : iter)
				new_length += elem.size() + size();
			new_length = std::max(0ll, new_length - size());

			self result(new_length, '\0');
			for (auto&& elem : iter)
			{
				for (c_size i = 0; i < elem.size(); ++i)
					result.__at__(pos++) = elem.__at__(i);

				for (c_size i = 0; i < size() && pos < new_length; ++i)
					result.__at__(pos++) = __at__(i);
			}

			return result;
		}

		self replace(const self& old_, const self& new_) const
		{

		}

		self split() const
		{

		}

		self split(const self& pattern) const
		{

		}

		self match(CharT lmatch, CharT rmatch) const
		{
			c_size l = find(lmatch, 0);
			if (l == -1)
				return "";

			c_size match_cnt = 0;
			for (c_size i = l; i < size(); ++i)
			{
				if (__at__(i) == lmatch)
					++match_cnt;
				else if (__at__(i) == rmatch)
					--match_cnt;

				if (match_cnt == 0)
					return slice(l, i + 1);
			}

			ValueError(std::format("Unmatched parentheses, too many left parentheses '{}'", lmatch));
		}


		void release()
		{
			if (-- (*ref_count_) == 0)
			{
				ayr_delloc(head_);
				ayr_delloc(ref_count_);
			}
		}
	};


	template<Char CharT>
	class Kmp : public Object
	{
		using self = Kmp;

		using Str_t = ayr::String<CharT>;

		Str_t kmp_str_;
	public:
		template<typename S>
		Kmp(S&& str) : kmp_str_(std::forward<S>(str)) {}
	};


	using Atring = String<char>;

	inline Atring operator ""as(const char* str, size_t len) { return Atring(str, len); }
}
#endif