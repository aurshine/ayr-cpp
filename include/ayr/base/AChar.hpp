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

		AChar() : byte_code_(nullptr), code_size_(0) {}

		AChar(char c) : byte_code_(ayr_alloc<char>(1)), code_size_(1) { byte_code_[0] = c; }

		AChar(const char* data_, Encoding* encoding)
		{
			code_size_ = encoding->byte_size(data_);
			byte_code_ = ayr_alloc<char>(code_size_);
			std::memcpy(data(), data_, code_size_);
		}

		AChar(int code, Encoding* encoding) : AChar(encoding->from_int(code).data(), encoding) {}

		AChar(const self& other)
		{
			code_size_ = other.size();
			byte_code_ = ayr_alloc<char>(code_size_);
			std::memcpy(data(), other.data(), code_size_);
		}

		AChar(self&& other) noexcept : byte_code_(std::exchange(other.byte_code_, nullptr)), code_size_(std::exchange(other.code_size_, 0)) {}

		~AChar() { ayr_delloc(byte_code_); code_size_ = 0; };

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(byte_code_);

			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(byte_code_);

			return *ayr_construct(this, std::move(other));
		}

		self& operator=(char c)
		{
			ayr_destroy(byte_code_);

			code_size_ = 1;
			byte_code_ = ayr_alloc<char>(1);
			byte_code_[0] = c;

			return *this;
		}

		char* data() noexcept { return byte_code_; }

		// 返回字节码
		const char* data() const noexcept { return byte_code_; }

		// 返回字节码长度
		c_size size() const noexcept { return code_size_; }

		CString __str__() const { return CString(data(), size()); }

		hash_t __hash__() const { return bytes_hash(data(), size()); }

		cmp_t __cmp__(const self& other)  const
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size)
				return m_size - o_size;
			else
				return std::memcmp(data(), other.data(), m_size);
		}

		void __swap__(self& other)
		{
			swap(byte_code_, other.byte_code_);
			swap(code_size_, other.code_size_);
		}

		operator char() const
		{
			if (!isasciii())
				EncodingError("AChar is not a single character");

			return byte_code_[0];
		}

		// 转换为整数
		int to_int(Encoding* encoding) const { return encoding->to_int(data(), size()); }

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
		char* byte_code_;

		int8_t code_size_;
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