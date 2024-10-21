#ifndef AYR_LAW_STRING_HPP
#define AYR_LAW_STRING_HPP


#include <law/printer.hpp>
#include <law/Dynarray.hpp>
#include <law/ayr_memory.hpp>


namespace ayr
{
	class Atring : public Sequence<char>
	{
		using self = Atring;

		using super = Sequence<char>;

		char* cstr_;

		c_size length_;

		std::shared_ptr<char[]> shared_head_;

		Atring(char c, c_size len)
		{
			length_ = len;
			shared_head_ = std::make_shared<char[]>(length_ + 1, c);
			cstr_ = shared_head_.get();
			length_ = len;
		}

	public:
		Atring() : Atring("", 0) {}

		Atring(self&& other) : Atring(other) {}

		Atring(const CString& other) : Atring(other.data(), other.size()) {}

		Atring(const char* str) : Atring(str, std::strlen(str)) {}

		Atring(const char* str, c_size len) : shared_head_(std::make_shared<char[]>(len + 1)), length_(len)
		{
			cstr_ = shared_head_.get();
			std::memcpy(cstr_, str, length_);
		}

		Atring(const self& other) : Atring(other.cstr_, other.length_) {}

		Atring(self& other) : cstr_(other.cstr_), length_(other.length_), shared_head_(other.shared_head_) {}

		self& operator= (self& other)
		{
			if (this == &other)
				return *this;

			ayr_construct(this, other);
			return *this;
		}

		self& operator= (self&& other) { return operator= (other); }

		self& operator= (const self& other)
		{
			if (this == &other)
				return *this;

			ayr_construct(this, other);
			return *this;
		}

		self operator+ (const self& other) const
		{
			self result('\0', size() + other.size());
			size_t self_size = size(), other_size = other.size();
			for (size_t i = 0; i < self_size; ++i)
				result[i] = at(i);

			for (size_t i = 0; i < other_size; ++i)
				result[self_size + i] = other[i];
			return result;
		}

		self operator+= (const self& othre)
		{
			self result = *this + othre;
			*this = std::move(result);
			return *this;
		}

		self operator* (size_t n)
		{
			self result('\0', size() * n);

			size_t pos = 0, self_size = size();
			while (n--)
				for (size_t i = 0; i < self_size; ++i)
					result.cstr_[pos++] = at(i);

			return result;
		}

		const char& operator[] (c_size index) const { return at(index); }

		char& operator[] (c_size index) { return at(index); }

		char& at(c_size index) { return cstr_[neg_index(index, size())]; }

		const char& at(c_size index) const { return cstr_[neg_index(index, size())]; }

		c_size size() const override { return length_; }

		hash_t __hash__() const { return bytes_hash(reinterpret_cast<const char*>(cstr_), length_ * sizeof(char)); }

		CString __str__() const { return CString(cstr_, length_); }

		c_size find(char c, c_size pos) const
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
					flag &= (at(i + j) == pattern.at(j));

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

		self slice(c_size start, c_size end) const
		{
			start = neg_index(start, size());
			end = neg_index(end, size());
			return Atring(cstr_ + start, end - start);
		}

		self slice(c_size start) const { return slice(start, size()); }

		bool startswith(const self& prefix) const
		{
			if (prefix.size() > size())
				return false;

			for (c_size i = 0; i < prefix.size(); ++i)
				if (prefix.at(i) != at(i))
					return false;

			return true;
		}

		bool endswith(const self& suffix) const
		{
			if (suffix.size() > size())
				return false;

			c_size suffix_size = suffix.size();
			for (c_size i = 0; i < suffix_size; ++i)
				if (suffix.at(i) != at(size() - suffix_size + i))
					return false;

			return true;
		}

		self upper() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.at(i) = std::toupper(result.at(i));
			return result;
		}

		self lower() const
		{
			self result = *this;
			for (c_size i = 0; i < size(); ++i)
				result.at(i) = std::tolower(result.at(i));
			return result;
		}

		self& strip_()
		{
			c_size l = 0, r = size();
			while (l < r && std::isspace(at(l))) ++l;
			while (l < r && std::isspace(at(r - 1))) --r;
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
			while (l < size() && std::isspace(at(l))) ++l;
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
			while (r > 0 && std::isspace(at(r - 1))) --r;
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
					result.at(pos++) = elem.at(i);

				for (c_size i = 0; i < size() && pos < new_length; ++i)
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

			self result('\0', size() + indices.size() * (new_.size() - old_.size()));
			for (c_size i = 0, j = 0, k = 0; i < size(); ++i)
			{
				if (j < indices.size() && i == indices[j])
				{
					for (auto&& c : new_)
						result.cstr_[k++] = c;
					i += old_.size() - 1;
					j++;
				}
				else
					result.cstr_[k++] = at(i);
			}

			return result;
		}

		self split() const
		{

		}

		self split(const self& pattern) const
		{

		}

		self match(char lmatch, char rmatch) const
		{
			c_size l = find(lmatch, 0);
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

			ValueError(std::format("Unmatched parentheses, too many left parentheses '{}'", lmatch));
		}
	};


	class Kmp : public Object<Kmp>
	{
		using self = Kmp;

		using Str_t = Atring;

		Str_t kmp_str_;
	public:
		template<typename S>
		Kmp(S&& str) : kmp_str_(std::forward<S>(str)) {}
	};


	inline Atring operator ""as(const char* str, size_t len) { return Atring(str, len); }
}
#endif