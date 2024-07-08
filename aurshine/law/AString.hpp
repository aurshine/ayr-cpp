#pragma once
#include <law/Array.hpp>
#include <law/CString.hpp>
#include <law/DynArray.hpp>
#include <law/printer.hpp>



namespace ayr
{
	template<Char T>
	class SubString;


	template<Char T>
	class AString : public Object
	{
	public:
		AString() {}

		AString(c_size size_, const T& ch) : astring_(size_, ch) {}

		AString(T* str) : astring_(strlen(str), str) {}

		AString(const T* str) : astring_(strlen(str)) { astring_.fill(str, str + size()); }

		AString(const CString& c_str) : AString(c_str.str) {}

		AString(CString&& c_str) : AString(c_str.str) { c_str.str = nullptr; }

		AString(const AString& other) : astring_(other.astring_) {}

		AString(AString&& other) : AString() { swap(other); }

		~AString() {}

		AString& operator=(const AString& other)
		{
			if (this != &other) astring_ = other.astring_;
			return *this;
		}

		AString& operator=(AString&& other)
		{
			if (this != &other) astring_ = std::move(other.astring_);
			return *this;
		}

		void swap(AString& other)
		{
			astring_.swap(other.astring_);
		}

		T& operator[] (c_size index) { return astring_[index]; }

		const T& operator[] (c_size index) const { return astring_[index]; }

		c_size size() const { return astring_.size_; }

		T* ptr() { return astring_.ptr(); }

		const T* ptr() const { return astring_.ptr(); }

		bool contains(const T& ch) const { return astring_.contains(ch); }

		bool contains(const AString& other) const { return find(other, 0) != -1; }

		int __cmp__(const AString& other) const { return astring_.__cmp__(other.astring_); }

		size_t __hash__() const { return char_hash(astring_.arr_); }

		const char* __str__() const
		{
			memcpy__str_buffer__(astring_.arr_, astring_.size_);
			return __str_buffer__;
		}

		c_size find(const T& ch, c_size pos = 0) const { return astring_.find(ch, pos); }

		c_size find(const std::function<bool(const T&)>& check, c_size pos = 0) const { return astring_.find(check, pos); }

		c_size find(const AString& other, c_size pos = 0) const
		{
			assert_insize(pos, 0, size() - 1);

			if (size() - pos < other.size()) return -1;

			for (c_size i = pos; i + other.size() <= size(); ++i)
			{
				bool flag = true;
				for (c_size j = 0; j < other.size(); ++j)
					if (astring_[i + j] != other.astring_[j])
					{
						flag = false;
						break;
					}

				if (flag) return i;
			}

			return -1;
		}

		DynArray<c_size> find_all(const AString& other, c_size pos = 0) const
		{
			DynArray<c_size> ret;

			while (pos < size())
			{
				pos = find(other, pos);
				if (pos != -1)
				{
					ret.append(pos);
					pos += other.size();
				}
				else break;
			}

			return ret;
		}

		// 切片[l, r]
		AString slice(c_size l, c_size r) const
		{
			AString ret;
			ret.astring_.swap(astring_.slice(l, r));

			return ret;
		}

		// 切片[l, r]
		SubString<T> subslice(c_size l, c_size r)
		{
			assert_insize(l, 0, size_ - 1);
			assert_insize(r, 0, size_ - 1);

			return SubString(astring_.arr_ + l, r - l + 1);
		}

		AString operator+(const AString& other) const
		{
			AString result{};
			Array<T> temp(astring_.size() + other.astring_.size());
			temp.fill(astring_);
			temp.fill(other.astring_, astring_.size());
			result.astring_ = std::move(temp);

			return result;
		}

		bool startwith(const AString& other) const
		{
			if (size() < other.size())	return false;

			for (c_size i = 0; i < other.size(); ++i)
				if (astring_[i] != other.astring_[i])
					return false;

			return true;
		}

		bool endwith(const AString& other) const
		{
			if (size() < other.size())	return false;
			for (c_size i = other.size() - 1; i >= 0; --i)
				if (astring_[i] != other.astring_[i])
					return false;

			return true;
		}

		AString upper() const
		{
			AString ret = *this;
			for (c_size i = 0; i < ret.size(); ++i)
				if (ret[i] >= 'a' && ret[i] <= 'z')
					ret[i] += 'A' - 'a';
			return ret;
		}

		AString lower() const
		{
			AString ret = *this;
			for (c_size i = 0; i < ret.size(); ++i)
				if (ret[i] >= 'A' && ret[i] <= 'Z')
					ret[i] += 'a' - 'A';
			return ret;
		}

		AString strip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(astring_[l])) l++;
			while (r > l && isspace(astring_[r - 1])) r--;

			return slice(l, r);
		}

		AString lstrip() const
		{
			c_size l = 0, r = size();
			while (l < r && isspace(astring_[l])) l++;

			return slice(l, r);
		}

		AString rstrip() const
		{
			c_size l = 0, r = size();
			while (r > l && isspace(astring_[r - 1])) r--;

			return slice(l, r);
		}

		AString join(const Array<AString>& join_strs) const
		{
			c_size ret_size = (join_strs.size() - 1) * size();
			for (c_size i = 0; i < join_strs.size(); ++i)
				ret_size += join_strs[i].size();

			Array<T> result(ret_size);
			for (c_size i = 0, j = 0; i < join_strs.size(); ++i)
			{
				if (i != 0)
				{
					result.fill(astring_, j);
					j += size();
				}

				result.fill(join_strs[i].astring_, j);
				j += join_strs[i].size();
			}

			AString ret;
			ret.astring_.swap(result);
			return ret;
		}

		AString replace(const AString& old_, const AString& new_) const
		{
			DynArray<c_size> poses = find_all(old_);

			Array<T> temp(size() + (new_.size() - old_.size()) * poses.size());

			// 当前走到原字符串的位置 以及 临时字符串的长度
			c_size cur_pos = 0, temp_length = 0;
			for (c_size i = 0; i < poses.size(); ++i)
			{
				temp.fill(astring_.arr_ + cur_pos, astring_.arr_ + poses[i], temp_length);
				temp_length += poses[i] - cur_pos;

				temp.fill(new_.astring_.arr_, new_.astring_.arr_ + new_.size(), temp_length);
				temp_length += new_.size();

				cur_pos = poses[i] + old_.size();
			}

			if (cur_pos < size())
				temp.fill(astring_.arr_ + cur_pos, astring_.arr_ + size(), temp_length);

			AString ret;
			ret.astring_.swap(temp);
			return ret;
		}

		Array<AString> split() const
		{
			DynArray<AString> das;

			c_size i = 0;
			while (i < size())
			{
				while (i < size() && isspace(astring_[i])) ++i;
				c_size j = i + 1;
				while (j < size() && !isspace(astring_[j])) ++j;
				das.append(slice(i, j));
				i = j;
			}

			return das.to_array();
		}

		Array<AString> split(const AString& other) const
		{
			DynArray<AString> das;
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

		T* begin() { return astring_.begin(); }

		T* end() { return astring_.end(); }

		const T* begin() const { return astring_.begin(); }

		const T* end() const { return astring_.end(); }

		std::reverse_iterator<T*> rbegin() { return astring_.rbegin(); }

		std::reverse_iterator<T*> rend() { return astring_.rend(); }

		const std::reverse_iterator<T*> rbegin() const { return astring_.rbegin(); }

		const std::reverse_iterator<T*> rend() const { return astring_.rend(); }
	private:
		Array<T> astring_;
	};

	using Astring = AString<char>;
}
