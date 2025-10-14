#ifndef AYR_BASE_CODEC_ACHAR_HPP
#define AYR_BASE_CODEC_ACHAR_HPP

#include "../raise_error.hpp"

namespace ayr
{
	class AChar : public Object<AChar>
	{
		using self = AChar;

		using super = Object<AChar>;

		// unicode序号
		int32_t unicode_;
	public:
		constexpr AChar() : unicode_(0) {}

		constexpr AChar(int32_t unicode) : unicode_(unicode) {}

		constexpr AChar(const self& other) : unicode_(other.unicode_) {}

		constexpr self& operator=(const self& other)
		{
			unicode_ = other.unicode_;
			return *this;
		}

		// 返回字符的unicode序号
		constexpr int32_t ord() const { return unicode_; }

		// 转为大写字母
		constexpr self upper() const
		{
			if (unicode_ >= 97 && unicode_ <= 122)
				return AChar(unicode_ - 32);
			else
				return *this;
		}

		// 转为小写字母
		constexpr self lower() const
		{
			if (unicode_ >= 65 && unicode_ <= 90)
				return AChar(unicode_ + 32);
			else
				return *this;
		}

		// 是否为空字符
		constexpr bool isspace() const { return (unicode_ >= 9 && unicode_ <= 13) || unicode_ == '\0' || unicode_ == ' '; }

		// 是否为数字
		constexpr bool isdigit() const { return unicode_ >= 48 && unicode_ <= 57; }

		// 是否为大写字母
		constexpr bool isupper() const { return unicode_ >= 65 && unicode_ <= 90; }

		// 是否为小写字母
		constexpr bool islower() const { return unicode_ >= 97 && unicode_ <= 122; }

		// 是否为字母
		constexpr bool isalpha() const { return isupper() || islower(); }

		constexpr cmp_t __cmp__(const self& other) const { return unicode_ - other.unicode_; }

		constexpr bool __equals__(const self& other) const { return unicode_ == other.unicode_; }

		constexpr hash_t __hash__() const { return unicode_; }

		void __repr__(Buffer& buffer) const;
	};
}
#endif // AYR_BASE_CODEC_ACHAR_HPP