#ifndef AYR_LAWLBYTES_HPP
#define AYR_LAWLBYTES_HPP

#include <cstdint>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>

namespace ayr
{
	using Byte = uint8_t;

	class ByteCode
	{
	public:
		constexpr ByteCode(const Byte* data, int8_t size) :byte_code_(), code_size_(0)
		{
			if (data == nullptr || size > 4 || size <= 0)
				ValueError("Invalid byte code data");

			byte_code_ = std::make_unique<Byte[]>(size);
			code_size_ = size;
			std::memcpy(byte_code_.get(), data, sizeof(Byte) * size);
		}

	private:
		std::unique_ptr<Byte[]> byte_code_;

		int8_t code_size_;
	};
}


#endif 