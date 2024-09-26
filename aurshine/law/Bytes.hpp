#ifndef AYR_LAWLBYTES_HPP
#define AYR_LAWLBYTES_HPP

#include <cstdint>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>

namespace ayr
{
	using Byte = uint8_t;

	struct Encodings
	{
		using EncodingType = uint16_t;

		constexpr static EncodingType UTF8 = 0x0001;

		constexpr static EncodingType UTF16 = 0x0002;

		constexpr static EncodingType UTF32 = 0x0004;

		constexpr static EncodingType ASCII = 0x0008;

		constexpr static EncodingType GB2312 = 0x0010;

		static constexpr int byte_count(const Byte* data, EncodingType encoding)
		{

		}

		static constexpr int _utf8_byte_count(const Byte* data)
		{
			if ((data[0] & 0x80) == 0)         // 以0    开头 (0xxxxxxx),1字节编码
				return 1;
			else if ((data[0] & 0xE0) == 0xC0) // 以110  开头 (110xxxxx),2字节编码
				return  2;
			else if ((data[0] & 0xF0) == 0xE0) // 以1110 开头 (1110xxxx),3字节编码
				return 3;
			else if ((data[0] & 0xF8) == 0xF0) // 以11110开头 (11110xxx),4字节编码
				return 4;
			else
				ValueError("Invalid code point");
			return 0;
		}

	};

	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<CodePoint>;

	public:
		CodePoint(const Byte* data, Encodings::EncodingType encoding) : encoding_(encoding)
		{
			int code_size = Encodings::byte_count(data, encoding_);
			byte_code_ = std::make_unique<Byte[]>(code_size);
			std::memcpy(byte_code_.get(), data, sizeof(Byte) * code_size);
		}

		CodePoint(const self& other) : encoding_(other.encoding_)
		{
			int code_size = other.size();
			byte_code_ = std::make_unique<Byte[]>(code_size);
			std::memcpy(byte_code_.get(), other.data(), sizeof(Byte) * code_size);
		}

		CodePoint(self&& other) noexcept : encoding_(other.encoding()), byte_code_(std::move(other.byte_code_)) {}

		~CodePoint() = default;

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other)
				return *this;

			return *ayr_construct(this, std::move(other));
		}

		const Byte* data() const noexcept { return byte_code_.get(); }

		const Encodings::EncodingType& encoding() const noexcept { return encoding_; }

		c_size size() const noexcept { return Encodings::byte_count(data(), encoding()); }

		CString __str__() const override { return CString(reinterpret_cast<const char*>(data()), size()); }

		hash_t __hash__() const override { return bytes_hash(reinterpret_cast<const char*>(data()), size()); }

		cmp_t __cmp__(const self& other)
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size)
				return m_size - o_size;

			return std::memcmp(data(), other.data(), m_size);
		}

	private:
		std::unique_ptr<Byte[]> byte_code_;

		Encodings::EncodingType encoding_;
	};
}


#endif 