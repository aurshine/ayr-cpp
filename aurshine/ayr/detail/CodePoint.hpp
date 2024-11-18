#ifndef AURSHINE_DETAIL_CODEPOINT_HPP
#define AURSHINE_DETAIL_CODEPOINT_HPP

#include <ayr/DynArray.hpp>
#include <ayr/detail/Encoding.hpp>

namespace ayr
{
	// 码点
	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<self>;

	public:
		static const Array<CodePoint> SPACE;

		static const Array<CodePoint> DIGIT;

		static const Array<CodePoint> LOWER_CASE_LETTER;

		static const Array<CodePoint> UPPER_CASE_LETTER;

		CodePoint() : byte_code_(nullptr), code_size_(0) {}

		CodePoint(char c) : byte_code_(ayr_alloc<char>(1)), code_size_(1) { byte_code_[0] = c; }

		CodePoint(const char* data_, Encoding* encoding)
		{
			code_size_ = encoding->byte_size(data_);
			byte_code_ = ayr_alloc<char>(code_size_);

			std::memcpy(data(), data_, code_size_);
		}

		CodePoint(const self& other)
		{
			code_size_ = other.size();
			byte_code_ = ayr_alloc<char>(code_size_);
			std::memcpy(data(), other.data(), code_size_);
		}

		CodePoint(self&& other) noexcept : byte_code_(other.byte_code_), code_size_(other.code_size_) { other.code_size_ = 0; other.byte_code_ = nullptr; }

		~CodePoint() { ayr_delloc(byte_code_); code_size_ = 0; };

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

		bool __equals__(const self& other) const { return __cmp__(other) == 0; }

		operator char() const { return byte_code_[0]; }

		self upper() const {}

		self lower() const {}

		bool isspace() const { return isasciii() && SPACE.contains(*this); }

		bool isalpha() const { return isasciii() && (UPPER_CASE_LETTER.contains(*this) || LOWER_CASE_LETTER.contains(*this)); }

		bool isdigit() const { return isasciii() && DIGIT.contains(*this); }

		bool isupper() const { return isasciii() && UPPER_CASE_LETTER.contains(*this); }

		bool islower() const { return isasciii() && LOWER_CASE_LETTER.contains(*this); }

		bool isasciii() const { return (unsigned)byte_code_[0] < 0x80; }
	private:
		char* byte_code_;

		int8_t code_size_;
	};

	const Array<CodePoint> CodePoint::SPACE = { '\0', '\t', '\n', '\r', '\v', '\f', ' ' };

	const Array<CodePoint> CodePoint::DIGIT = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	const Array<CodePoint> CodePoint::LOWER_CASE_LETTER = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l','m', 'n', 'o', 'p', 'q', 'r','s', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

	const Array<CodePoint> CodePoint::UPPER_CASE_LETTER = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

	// 获取字符串的所有码点
	def get_cps(const char* data, c_size len, Encoding* encoding)
	{
		DynArray<CodePoint> cps;
		int i = 0;

		while (i < len)
		{
			cps.append(CodePoint(data + i, encoding));
			i += cps[-1].size();
			if (i > len) EncodingError("Invalid encoding, code point out of range");
		}
		return cps;
	}
}
#endif