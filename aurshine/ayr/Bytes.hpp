#ifndef AYRLBYTES_HPP
#define AYRLBYTES_HPP

#include <ayr/detail/Encoding.hpp>
#include <ayr/Dict.hpp>


namespace ayr
{
	class CodePoint;



	Encoding* encoding_map(const CString& encoding_name)
	{
		static Dict<CString, Encoding*> encodingMap_{
			{ASCII, new ASCIIEncoding()},
			{UTF8, new UTF8Encoding()},
			{UTF16, new UTF16Encoding()},
			{UTF32, new UTF32Encoding()},
			{GB2312, new GB2312Encoding()}
		};

		return encodingMap_.get(encoding_name);
	}

	const Array<CodePoint> SPACE = { CodePoint("\0"),
									 CodePoint(" "),
									 CodePoint("\t"),
									 CodePoint("\n"),
									 CodePoint("\r"),
									 CodePoint("\v"),
									 CodePoint("\f") };

	const Array<CodePoint> DIGIT = { CodePoint("0"),
									 CodePoint("1"),
									 CodePoint("2"),
									 CodePoint("3"),
									 CodePoint("4"),
									 CodePoint("5"),
									 CodePoint("6"),
									 CodePoint("7"),
									 CodePoint("8"),
									 CodePoint("9") };

	const Array<CodePoint> LOWER_CASE_LETTER = { CodePoint("a"),
												 CodePoint("b"),
												 CodePoint("c"),
												 CodePoint("d"),
												 CodePoint("e"),
												 CodePoint("f"),
												 CodePoint("g"),
												 CodePoint("h"),
												 CodePoint("i"),
												 CodePoint("j"),
												 CodePoint("k"),
												 CodePoint("l"),
												 CodePoint("m"),
												 CodePoint("n"),
												 CodePoint("o"),
												 CodePoint("p"),
												 CodePoint("q"),
												 CodePoint("r"),
												 CodePoint("s"),
												 CodePoint("t"),
												 CodePoint("u"),
												 CodePoint("v"),
												 CodePoint("w"),
												 CodePoint("x"),
												 CodePoint("y"),
												 CodePoint("z") };

	const Array<CodePoint> UPPER_CASE_LETTER = { CodePoint("A"),
												 CodePoint("B"),
												 CodePoint("C"),
												 CodePoint("D"),
												 CodePoint("E"),
												 CodePoint("F"),
												 CodePoint("G"),
												 CodePoint("H"),
												 CodePoint("I"),
												 CodePoint("J"),
												 CodePoint("K"),
												 CodePoint("L"),
												 CodePoint("M"),
												 CodePoint("N"),
												 CodePoint("O"),
												 CodePoint("P"),
												 CodePoint("Q"),
												 CodePoint("R"),
												 CodePoint("S"),
												 CodePoint("T"),
												 CodePoint("U"),
												 CodePoint("V"),
												 CodePoint("W"),
												 CodePoint("X"),
												 CodePoint("Y"),
												 CodePoint("Z") };

	// 码点
	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<self>;

	public:
		CodePoint() : byte_code_(nullptr), code_size_(0) {}

		CodePoint(char c) : byte_code_(std::make_unique<char[]>(1)), code_size_(1) { byte_code_[0] = c; }

		CodePoint(const char* bytes, const CString& encoding = "ascii")
		{
			code_size_ = encoding_map(encoding)->byte_size(bytes);
			byte_code_ = std::make_unique<char[]>(code_size_);
			std::memcpy(byte_code_.get(), bytes, code_size_);
		}

		CodePoint(const self& other)
		{
			code_size_ = other.size();
			byte_code_ = std::make_unique<char[]>(code_size_);
			std::memcpy(byte_code_.get(), other.bytes(), code_size_);
		}

		CodePoint(self&& other) noexcept : byte_code_(std::move(other.byte_code_)), code_size_(other.code_size_) {}

		~CodePoint() = default;

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			code_size_ = other.code_size_;
			byte_code_ = std::make_unique<char[]>(code_size_);
			std::memcpy(byte_code_.get(), other.bytes(), code_size_);

			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			code_size_ = other.code_size_;
			byte_code_ = std::move(other.byte_code_);

			return *this;
		}

		// 返回字节码
		const char* bytes() const noexcept { return byte_code_.get(); }

		// 返回字节码长度
		c_size size() const noexcept { return code_size_; }

		CString __str__() const { return CString(reinterpret_cast<const char*>(bytes()), size()); }

		hash_t __hash__() const { return bytes_hash(reinterpret_cast<const char*>(bytes()), size()); }

		cmp_t __cmp__(const self& other)  const
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size)
				return m_size - o_size;
			else
				return std::memcmp(bytes(), other.bytes(), m_size);
		}

		bool __equals__(const self& other) const { return __cmp__(other) == 0; }

		self upper() const {}

		self lower() const {}

		bool isspace() const { return isasciii() && SPACE.contains(*this); }

		bool isalpha() const { return isasciii() && (UPPER_CASE_LETTER.contains(*this) || LOWER_CASE_LETTER.contains(*this);) }

		bool isdigit() const { return isasciii() && DIGIT.contains(*this); }

		bool isupper() const { return isasciii() && UPPER_CASE_LETTER.contains(*this); }

		bool islower() const { return isasciii() && LOWER_CASE_LETTER.contains(*this); }

		bool isasciii() const { return (unsigned)byte_code_[0] < 0x80; }
	private:
		std::unique_ptr<char[]> byte_code_;

		int8_t code_size_;
	};
}


#endif 