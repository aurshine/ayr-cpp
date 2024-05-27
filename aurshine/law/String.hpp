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

		bool contains(const CharT& ch) const { return astring_.contains(ch); }

		bool contains(const AString& other) const
		{

		}

		CharT& operator[] (c_size index) { return astring_[index]; }

		const CharT& operator[] (c_size index) const { return astring_[index]; }

		AString slice(c_size l, c_size r) { return ASlice(astring_.slice(l, r)); }

		int __cmp__(const AString& other) const { return astring_.__cmp__(other.astring_); }

		std::basic_string<CharT> __str__() const override
		{
			std::basic_string<CharT> str;
			for (c_size i = 0; i < size(); i++)
				str.push_back(astring_[i]);

			return str;
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

		}

		bool endwith(const AString& other) const
		{

		}

		AString replace(const AString& old, const AString& new_) const
		{

		}

		AString upper() const
		{

		}

		AString lower() const
		{

		}

		AString strip() const
		{

		}

		AString lstrip() const
		{

		}

		AString rstrip() const
		{

		}

		AString split(const AString& other) const
		{

		}
	private:
		Array<CharT> astring_;
	};
}
