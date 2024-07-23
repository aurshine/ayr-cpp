#pragma once
#include <law/detail/Array.hpp>
#include <law/detail/CString.hpp>
#include <law/detail/printer.hpp>
#include <law/DynArray.hpp>


namespace ayr
{
	template<Char T>
	class SubString;


	template<Char Ch>
	class AString : public IndexContainer<AString<Ch>, Ch>
	{
		using self = AString<Ch>;

		using super = IndexContainer<AString<Ch>, Ch>;
	public:
		using Char_t = Ch;

		AString(): str_() {}

		AString(const Char_t* str) : str_(str), size_(str_.size()) {}

		AString(CString&& c_str) : str_(std::move(c_str)), size_(str_.size()) {}

		AString(const CString& c_str) : AString(c_str.str) {}

		AString(const self& other) : AString(other.str_) {}

		AString(self&& other) : AString(std::move(other.str_)) {}

		~AString() {}

		AString& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			str_ = other.str_;
			size_ = other.size_;
			return *this;
		}

		AString& operator=(self&& other)
		{
			if (this == &other)
				return *this;

			str_ = std::move(other.str_);
			size_ = other.size_;
			other.size_ = 0;
			return *this;
		}

		void swap(self& other) 
		{ 
			std::swap(str_.str, other.str_.str); 
			std::swap(size_, other.size_);
		}

		Char_t& operator[] (c_size index)
		{
			index = (index + size_) % size_;
			return str_[index]; 
		}

		const Char_t& operator[] (c_size index) const
		{ 
			index = (index + size_) % size_;
			return str_[index];
		}

		c_size size() const { return size_; }

		Char_t* ptr() { return str_.str; }

		const Char_t* ptr() const { return str_.str; }

		bool contains(const Char_t& ch) const { return find(ch, 0) != -1; }

		bool contains(const self& other) const { return find(other, 0) != -1; }

		int __cmp__(const self& other) const { return str_.__cmp__(other.str_); }

		size_t __hash__() const { return str_.__hash__(); }

		virtual self& __iter_container__() const { return const_cast<self&>(*this); }

		CString __str__() const { return str_; }

		SubString<Char_t> subme() { return SubString<Char_t>(ptr(), size_); }

		c_size find(const Char_t& ch, c_size pos = 0) const 
		{ 
			for (c_size i = pos; i < size_; ++ i)
				if (str_[i] == ch)
					return i;
			return -1;
		}

		c_size find(const self& other, c_size pos = 0) const
		{
			if (size() < other.size() + pos) return -1;

			SubString<Char_t> sub_other = other.subme();
			for (c_size i = pos; i + other.size() <= size(); ++i)
				if (subslice(pos, pos + other.size()) == sub_other)
					return i;

			return -1;
		}

		Array<c_size> find_all(const self& other, c_size pos = 0) const
		{
			DynArray<c_size> ret;

			while (pos != -1)
			{
				pos = find(other, pos);
				if (pos != -1)
				{
					ret.append(pos);
					pos += other.size();
				}
			}

			return ret.to_array();
		}

		// 切片[l, r)
		self slice(c_size l, c_size r) const
		{
			RawString<Char_t> rs(r - l + 1);
			
			while (l < r)
			{
				static c_size i = 0;
				rs[i++] = str_[l++];
			}

			return AString(rs);
		}

		// 切片[l, r)
		SubString<Char_t> subslice(c_size l, c_size r) { return SubString(ptr() + l, r - l); }

		self operator+(const self& other) const
		{
			RawString<Char_t> rs(size() + other.size() + 1);
			for (c_size i = 0; i < size(); ++i)
				rs[i] = str_[i];
			for (c_size i = 0; i < other.size(); ++i)
				rs[i + size()] = other[i];

			return AString(rs);
		}

		self operator*(c_size n) const
		{
			RawString<Char_t> rs(size() * n);
			for (c_size i = 0; i < n; ++i)
				rs[i] = str_[i % n];

			return AString(rs);
		}

		bool startwith(const self& other) const
		{
			if (size() < other.size())	return false;

			return find(other, 0) == 0;
		}

		bool endwith(const self& other) const
		{
			if (size() < other.size())	return false;
			
			return subslice(size() - other.size(), size()) == other.subme();
		}

		self upper() const
		{
			self ret = *this;
			for (c_size i = 0; i < ret.size(); ++i)
				if (ret[i] >= 'a' && ret[i] <= 'z')
					ret[i] += 'A' - 'a';
			return ret;
		}

		self lower() const
		{
			self ret = *this;
			for (c_size i = 0; i < ret.size(); ++i)
				if (ret[i] >= 'A' && ret[i] <= 'Z')
					ret[i] += 'a' - 'A';
			return ret;
		}

		self strip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(astring_[l])) l++;
			while (r > l && isspace(astring_[r - 1])) r--;

			return slice(l, r);
		}

		self lstrip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(str_[l])) l++;

			return slice(l, r);
		}

		self rstrip() const
		{
			c_size l = 0, r = size();
			while (r > l && isspace(str_[r - 1])) r--;

			return slice(l, r);
		}

		self join(const Array<self>& join_strs) const
		{
			c_size ret_size = (join_strs.size() - 1) * size();
			for (c_size i = 0; i < join_strs.size(); ++i)
				ret_size += join_strs[i].size();

			RawString<Char_t> rs(ret_size);
			for (c_size i = 0, j = 0; i < join_strs.size(); ++i)
			{
				if (i != 0)
					for (c_size k = 0; k < size(); ++k)
						rs[j++] = str_[k];

				for (c_size k = 0; k < join_strs[i].size(); ++k)
					rs[j++] = join_strs[i].str_[k];
			}

			return AString(rs);
		}

		self replace(const self& old_, const self& new_) const
		{
			DynArray<c_size> poses = find_all(old_);

			RawString<Char_t> rs{ size() + (new_.size() - old_.size()) * poses.size() };

			c_size pos_idx = 0;
			for (c_size i = 0, j = 0; i < size(); ++i)
			{
				if (poses[pos_idx] == i)
				{
					++ pos_idx;
					for (c_size k = 0; k < new_.size(); ++k)
						rs[j++] = new_[k];
					i += old_.size() - 1;
				}
				else
				{
					rs[j++] = str_[i];
				}
			}

			return AString(rs);
		}

		Array<self> split() const
		{
			DynArray<self> das;

			c_size i = 0;
			while (i < size())
			{
				while (i < size() && isspace(str_[i])) ++i;
				c_size j = i + 1;
				while (j < size() && !isspace(str_[j])) ++j;
				das.append(slice(i, j));
				i = j;
			}

			return das.to_array();
		}

		Array<self> split(const self& other) const
		{
			DynArray<self> das;
			c_size last_pos = 0;

			for (auto pos : find_all(other))
			{
				if (last_pos != pos)
					das.append(slice(last_pos, pos));

				last_pos = pos + other.size();
			}

			if (last_pos < size())
				das.append(slice(last_pos, size()));

			return das.to_array();
		}
	private:
		RawString<Char_t> str_;

		c_size size_;
	};

	using Astring = AString<char>;
}
