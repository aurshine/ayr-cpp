#pragma once
#include <law/AString.hpp>

namespace ayr
{
	template<Char Ch>
	class SubString : public IndexContainer<SubString<Ch>, Ch>
	{
		using self = SubString<Ch>;
	public:
		using Char_t = Ch;

		SubString(const Char_t* str, size_t size) : substr_(str), size_(size) {}
		
		SubString(const SubString& other) : substr_(other.substr_), size_(other.size_) {}

		SubString& operator=(const SubString& other)
		{
			substr_ = other.substr_;
			size_ = other.size_;
			return *this;
		}

		void swap(SubString& other)
		{
			std::swap(substr_, other.substr_);
			std::swap(size_, other.size_);
		}

		Char_t& operator[] (c_size index) { return substr_[index]; }

		const Char_t& operator[] (c_size index) const { return substr_[index]; }

		c_size size() const { return substr_.size_; }

		Char_t* ptr() { return substr_; }

		const Char_t* ptr() const { return substr_; }

		SubString slice(c_size l, c_size r) const { return SubString(substr_ + l, r - l + 1); }

		CString __str__() const { return CString(reinterpret_cast<char*>(substr_), size_ * sizeof(Char_t)); }

		cmp_t __cmp__(const SubString& other) const
		{
			for (size_t i = 0; substr_[i] || other.substr_[i]; i++)
				if (substr_[i] != other.substr_[i])
					return substr_[i] - other.substr_[i];
			return 0;
		}

		size_t __hash__() const { return bytes_hash(substr_, size_); }

		self& __iter_container__() const override { return const_cast<self&>(*this); }

		bool contains(const Char_t& ch) const { return find(ch, 0) != -1; }

		bool contains(const self& other) const { return find(other, 0) != -1; }

		AString<Char_t> strme() const { return AString<Char_t>(RawString<Char_t>(substr_, size_)); }

		c_size find(const Char_t& ch, c_size pos = 0) const
		{
			for (c_size i = pos; i < size_; ++i)
				if (substr_[i] == ch)
					return i;
			return -1;
		}

		c_size find(const self& other, c_size pos = 0) const
		{
			if (size() < other.size() + pos) return -1;

			for (c_size i = pos; i + other.size() <= size(); ++i)
				if (slice(pos, pos + other.size()) == other)
					return i;

			return -1;
		}

		bool startwith(const self& other) const
		{
			if (size() < other.size())	return false;

			return find(other, 0) == 0;
		}

		bool endwith(const self& other) const
		{
			if (size() < other.size())	return false;

			return slice(size() - other.size(), size()) == other;
		}

		self strip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(substr_[l])) l++;
			while (r > l && isspace(substr_[r - 1])) r--;

			return slice(l, r);
		}

		self lstrip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(substr_[l])) l++;

			return slice(l, r);
		}

		self rstrip() const
		{
			c_size l = 0, r = size();
			while (r > l && isspace(substr_[r - 1])) r--;

			return slice(l, r);
		}


		self match(const Char_t& l_match, const Char_t& r_match) const
		{
			c_size l = find(l_match) + 1, match_cnt = 1;
			for (int i = l; i < size(); ++i)
			{
				if (substr_[i] == l_match)
					++match_cnt;
				else if (substr_[i] == r_match)
					--match_cnt;

				if (match_cnt == 0)
					return slice(l, i);
			}

			ValueError("string is not matched");
		}
	private:
		Ch* substr_;

		size_t size_;
	};
	
	using Str = SubString<char>;
}