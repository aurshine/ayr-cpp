#pragma once
#include <cstring>
#include "printer.hpp"
#include "Array.hpp"


namespace ayr
{
	template<typename CharT>
	class AString : public Object
	{
	public:
		AString() {}

		AString(const CharT& ch, c_size size_ = 1) : astring_(ch, size_) {}

		AString(CharT* str) : astring_(str, strlen(str)) {}

		AString(const CharT* str) : astring_(strlen(str)) { astring_.fill(str, size()); }

		AString(const AString& other) : astring_(other.astring_) {}

		AString(AString&& other) : astring_(std::move(other.astring_)) {}

		~AString() {}

		AString& operator=(const AString& other)
		{
			astring_ = other.astring_;
			return *this;
		}

		AString& operator=(AString&& other)
		{
			astring_ = std::move(other.astring_);
			return *this;
		}

		c_size size() const { return astring_.size(); }

		CharT* ptr() { return astring_.ptr(); }

		const CharT* ptr() const { return astring_.ptr(); }

		bool contains(const CharT& ch) const { return astring_.contains(ch); }

		bool contains(const AString& other) const { return find(other, 0) != -1; }

		c_size find(const CharT& ch, c_size pos = 0) const { return astring_.find(ch, pos); }

		c_size find(const std::function<bool(const CharT&)>& check, c_size pos = 0) const { return astring_.find(check, pos); }

		c_size find(const AString& other, c_size pos = 0) const
		{
			assert_insize(pos, 0, size() - 1);
			
			if (size() - pos < other.size()) return -1;
			for (c_size i = pos; i + other.size() < size(); ++i)
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

		CharT& operator[] (c_size index) { return astring_[index]; }

		const CharT& operator[] (c_size index) const { return astring_[index]; }

		AString slice(c_size l, c_size r) const 
		{ 
			AString ret;
			ret.astring_ = astring_.slice(l, r);
			
			return ret;
		}

		int __cmp__(const AString& other) const { return astring_.__cmp__(other.astring_); }

		const char* __str__() const override
		{
			std::basic_string<CharT> str;
			for (c_size i = 0; i < size(); i++)
				str.push_back(astring_[i]);

			return str.c_str();
		}

		AString operator+(const AString& other) const
		{
			AString result{};
			Array<CharT> temp(astring_.size() + other.astring_.size());
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
			for (c_size i = other.size() - 1; i >= 0; -- i)
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
			while (r > l && isspace(astring_[r - 1])) r --;
			
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
				
			Array<CharT> result(ret_size);
			AString ret;

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

			ret.astring_ = std::move(result);
			return ret;
		}

		AString replace(const AString& old_, const AString& new_) const
		{

		}

		Array<AString> split() const
		{

		}

		Array<AString> split(const AString& other) const
		{

		}
	private:
		Array<CharT> astring_;
	};
}
