#ifndef AURSHINE_BASE_CODEPOINT_HPP
#define AURSHINE_BASE_CODEPOINT_HPP

#include "Encoding.hpp"
#include "raise_error.hpp"
#include "../DynArray.hpp"

namespace ayr
{
	// 码点
	class AChar : public Object<AChar>
	{
		using self = AChar;

		using super = Object<self>;

	public:
		static const Array<AChar> SPACE;

		static const Array<AChar> DIGIT;

		static const Array<AChar> LOWER_CASE_LETTER;

		static const Array<AChar> UPPER_CASE_LETTER;

		AChar() { std::memset(byte_code_, 0, sizeof(byte_code_)); }

		AChar(char c) : AChar() { byte_code_[0] = c; }

		AChar(const char* data_, Encoding* encoding) : AChar()
		{
			int code_size = encoding->byte_size(data_);
			std::memcpy(byte_code_, data_, code_size);
		}

		AChar(int code, Encoding* encoding) : AChar(encoding->chr(code).data(), encoding) {}

		AChar(const self& other) { std::memcpy(byte_code_, other.byte_code_, sizeof(byte_code_)); }

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			std::memcpy(byte_code_, other.byte_code_, sizeof(byte_code_));
			return *this;
		}

		self& operator=(char c)
		{
			std::memset(byte_code_, 0, sizeof(byte_code_));
			byte_code_[0] = c;
			return *this;
		}

		// 返回字节码长度
		c_size size() const noexcept
		{
			c_size size = 0;
			while (size < 4 && byte_code_[size] != 0) ++size;
			return size;
		}

		CString __str__() const { return dstr(byte_code_, size()); }

		void __repr__(Buffer& buffer) const { buffer.append_bytes(byte_code_, size()); }

		hash_t __hash__() const { return bytes_hash(byte_code_, size()); }

		cmp_t __cmp__(const self& other)  const
		{
			return std::memcmp(byte_code_, other.byte_code_, sizeof(byte_code_));
		}

		cmp_t __cmp__(char c) const
		{
			if (byte_code_[0] == c)
				return byte_code_[1];
			return byte_code_[0] - c;
		}

		operator char() const
		{
			if (!isasciii())
				EncodingError("AChar is not a single character");

			return byte_code_[0];
		}

		// 转换为整数
		int ord(Encoding* encoding) const { return encoding->ord(byte_code_, size()); }

		// 转换为大写字母
		self upper() const
		{
			if (islower()) return byte_code_[0] - 'a' + 'A';
			return *this;
		}

		// 转换为小写字母
		self lower() const
		{
			if (isupper()) return byte_code_[0] - 'A' + 'a';
			return *this;
		}

		// 是否为空白字符
		bool isspace() const { return isasciii() && SPACE.contains(*this); }

		// 是否为字母
		bool isalpha() const { return isasciii() && (UPPER_CASE_LETTER.contains(*this) || LOWER_CASE_LETTER.contains(*this)); }

		// 是否为数字
		bool isdigit() const { return isasciii() && DIGIT.contains(*this); }

		// 是否为大写字母
		bool isupper() const { return isasciii() && UPPER_CASE_LETTER.contains(*this); }

		// 是否为小写字母
		bool islower() const { return isasciii() && LOWER_CASE_LETTER.contains(*this); }

		// 是否为ASCII字符
		bool isasciii() const { return byte_code_[0] < 0x80; }
	private:
		char byte_code_[4];
	};

	const Array<AChar> AChar::SPACE = { '\0', '\t', '\n', '\r', '\v', '\f', ' ' };

	const Array<AChar> AChar::DIGIT = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	const Array<AChar> AChar::LOWER_CASE_LETTER = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l','m', 'n', 'o', 'p', 'q', 'r','s', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

	const Array<AChar> AChar::UPPER_CASE_LETTER = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

	// 获取字符串的所有码点
	def get_cps(const char* data, c_size len, Encoding* encoding)
	{
		DynArray<AChar> cps;
		int i = 0;

		while (i < len)
		{
			cps.append(AChar(data + i, encoding));
			i += cps[-1].size();
			if (i > len) EncodingError("Invalid encoding, code point out of range");
		}
		return cps;
	}
}
#endif