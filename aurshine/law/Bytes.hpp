#ifndef AYR_LAWLBYTES_HPP
#define AYR_LAWLBYTES_HPP

#include <cstdint>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>

namespace ayr
{
	using Byte = char;

	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<CodePoint>;

	public:
		constexpr CodePoint(const Byte* data)
		{

			if ((data[0] & 0x80) == 0)         // 以0    开头 (0xxxxxxx),1字节编码
				code_size_ = 1;
			else if ((data[0] & 0xE0) == 0xC0) // 以110  开头 (110xxxxx),2字节编码
				code_size_ = 2;
			else if ((data[0] & 0xF0) == 0xE0) // 以1110 开头 (1110xxxx),3字节编码
				code_size_ = 3;
			else if ((data[0] & 0xF8) == 0xF0) // 以11110开头 (11110xxx),4字节编码
				code_size_ = 4;
			else
				ValueError("Invalid code point");

			byte_code_ = std::make_unique<Byte[]>(code_size_);
			std::memcpy(byte_code_.get(), data, sizeof(Byte) * code_size_);
		}

		c_size size() const noexcept { return code_size_; }

		const Byte* data() const noexcept { return byte_code_.get(); }

		CString __str__() const override { return CString(data(), size()); }

		hash_t __hash__() const override { return bytes_hash(data(), size()); }

		cmp_t __cmp__(const self& other)
		{
			if (size() != other.size())
				return size() - other.size();

			return std::memcmp(data(), other.data(), size());
		}

	private:
		std::unique_ptr<Byte[]> byte_code_;

		int8_t code_size_;
	};
}


#endif 