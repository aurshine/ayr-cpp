#ifndef AYR_BASE_CODEC_UNICODEC_HPP
#define AYR_BASE_CODEC_UNICODEC_HPP

#include "AChar.hpp"

namespace ayr
{
	// unicode编解码器
	template<typename TC>
	concept UniCodec = requires(
		const TC & codec,
		const char* bytes,
		AChar * code,
		c_size size,
		Buffer & buffer,
		const CString & data
		)
	{
		{ codec.first_char_size(bytes) } -> std::same_as<int>;
		{ codec.encode(code, 0, buffer) };
		{ codec.decode(code, 0, data) };
	};

	// UTF-8编解码器
	class UTF8Codec : public Object<UTF8Codec>
	{
		using self = UTF8Codec;

		using super = Object<self>;
	public:
		constexpr UTF8Codec() {}

		constexpr UTF8Codec(const self&) {}

		constexpr self& operator=(const self&) { return *this; }

		// 字节串的第一个字符的字节数
		// 如果是非法字节串, 返回-1
		constexpr int first_char_size(const char* bytes) const
		{
			if ((bytes[0] & 0x80) == 0)         // 以0    开头 1字节编码 (0xxxxxxx)
				return 1;
			else if ((bytes[0] & 0xE0) == 0xC0) // 以110  开头 2字节编码(110xxxxx 10xxxxxx)
				return  2;
			else if ((bytes[0] & 0xF0) == 0xE0) // 以1110 开头 3字节编码 (1110xxxx 10xxxxxx 10xxxxxx)
				return 3;
			else if ((bytes[0] & 0xF8) == 0xF0) // 以11110开头 4字节编码 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx),
				return 4;
			else
				return -1;
		}

		/*
		* @brief 将AChar序列编码为字节串, 编码后的字节串放入buffer中
		*
		* @param code 待编码的AChar序列
		*
		* @param size code的有效长度
		*
		* @param buffer 编码后的字节串
		*/
		void encode(const AChar* code, c_size size, Buffer& buffer) const
		{
			for (c_size i = 0; i < size; ++i)
			{
				uint32_t unicode = code[i].ord();
				if (unicode < 0 || unicode > 0x10FFFF)
					EncodingError(std::format("Invalid AChar: {}", unicode));

				if (unicode <= 0x7F)
				{
					buffer << static_cast<char>(unicode);
				}
				else if (unicode <= 0x7FF)
				{
					buffer << static_cast<char>(0xC0 | (unicode >> 6));
					buffer << static_cast<char>(0x80 | (unicode & 0x3F));
				}
				else if (unicode <= 0xFFFF)
				{
					buffer << static_cast<char>(0xE0 | (unicode >> 12));
					buffer << static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
					buffer << static_cast<char>(0x80 | (unicode & 0x3F));
				}
				else if (unicode <= 0x10FFFF)
				{
					buffer << static_cast<char>(0xF0 | (unicode >> 18));
					buffer << static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
					buffer << static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
					buffer << static_cast<char>(0x80 | (unicode & 0x3F));
				}
			}
		}

		/*
		* @brief 将字节串解码为AChar序列, 解码后的AChar序列放入dst中
		*
		* @param dst 解码后的AChar序列
		*
		* @param size dst的有效长度
		*
		* @param bytes 待解码的字节串
		*/
		constexpr void decode(AChar* dst, c_size size, const CString& bytes) const
		{
			const char* bytes_ptr = bytes.data();
			for (c_size i = 0, offset = 0; i < size && offset < bytes.size(); ++i)
			{
				int char_size = first_char_size(bytes_ptr + offset);
				switch (char_size)
				{
				case 1: dst[i] = bytes_ptr[offset]; break;
				case 2: dst[i] = ((bytes_ptr[offset] & 0x1F) << 6) | (bytes_ptr[offset + 1] & 0x3F); break;
				case 3: dst[i] = ((bytes_ptr[offset] & 0x0F) << 12) | ((bytes_ptr[offset + 1] & 0x3F) << 6) | (bytes_ptr[offset + 2] & 0x3F); break;
				case 4: dst[i] = ((bytes_ptr[offset] & 0x07) << 18) | ((bytes_ptr[offset + 1] & 0x3F) << 12) | ((bytes_ptr[offset + 2] & 0x3F) << 6) | (bytes_ptr[offset + 3] & 0x3F); break;
				default: EncodingError("Invalid Byte Sequence");
				}
				offset += char_size;
			}
		}
	};

	// UTF-16编解码器
	class UTF16Codec : public Object<UTF16Codec>
	{
		using self = UTF16Codec;

		using super = Object<self>;
	public:
		constexpr UTF16Codec() {}

		constexpr UTF16Codec(const self&) {}

		constexpr self& operator=(const self&) { return *this; }

		// 字节串的第一个字符的字节数
		// 如果是非法字节串, 返回-1
		constexpr int first_char_size(const char* bytes) const
		{
			// 检查是否是代理对
			uint16_t first = (bytes[0] << 8) | bytes[1];
			if (first >= 0xD800 && first <= 0xDBFF) // 高位代理
			{
				uint16_t second = (bytes[2] << 8) | bytes[3];
				if (second >= 0xDC00 && second <= 0xDFFF) // 低位代理
					return 4;
			}
			return 2; // BMP 范围
		}

		/*
		* @brief 将AChar序列编码为字节串, 编码后的字节串放入buffer中
		*
		* @param code 待编码的AChar序列
		*
		* @param size code的有效长度
		*
		* @param buffer 编码后的字节串
		*/
		void encode(const AChar* code, c_size size, Buffer& buffer) const
		{
			for (c_size i = 0; i < size; ++i)
			{
				uint32_t unicode = code[i].ord();
				if (unicode < 0 || unicode > 0x10FFFF)
					EncodingError(std::format("Invalid AChar: {}", unicode));

				if (unicode <= 0xFFFF)
				{
					buffer << static_cast<char>(unicode >> 8);
					buffer << static_cast<char>(unicode & 0xFF);
				}
				else if (unicode <= 0x10FFFF)
				{
					unicode -= 0x10000;
					buffer << static_cast<char>(0xD8 | (unicode >> 18));
					buffer << static_cast<char>(0xDC | ((unicode >> 10) & 0x3F));
					buffer << static_cast<char>(0xDC | (unicode & 0x3F));
					buffer << static_cast<char>(0xDC | ((unicode >> 22) & 0x3F));
				}
			}
		}

		/*
		* @brief 将字节串解码为AChar序列, 解码后的AChar序列放入dst中
		*
		* @param dst 解码后的AChar序列
		*
		* @param size dst的有效长度
		*
		* @param bytes 待解码的字节串
		*/
		constexpr void decode(AChar* dst, c_size size, const CString& bytes) const
		{
			const char* bytes_ptr = bytes.data();
			for (c_size i = 0, offset = 0; i < size && offset < bytes.size(); ++i)
			{
				int char_size = first_char_size(bytes_ptr + offset);
				switch (char_size)
				{
				case 2: dst[i] = (bytes_ptr[offset] << 8) | bytes_ptr[offset + 1]; break;
				case 4:
					dst[i] = 0x10000 + (((bytes_ptr[offset] << 8 | bytes_ptr[offset + 1]) - 0xD800) << 10) + (bytes_ptr[offset + 2] << 8 | bytes_ptr[offset + 3]) - 0xDC00;
					break;
				default: EncodingError("Invalid Byte Sequence");
				}
				offset += char_size;
			}
		}
	};

	// UTF-32编解码器
	class UTF32Codec : public Object<UTF32Codec>
	{
		using self = UTF32Codec;

		using super = Object<self>;
	public:
		constexpr UTF32Codec() {}

		constexpr UTF32Codec(const self&) {}

		constexpr self& operator=(const self&) { return *this; }

		// 字节串的第一个字符的字节数
		// 如果是非法字节串, 返回-1
		constexpr int first_char_size(const CString& bytes) const { return 4; }

		/*
		* @brief 将AChar序列编码为字节串, 编码后的字节串放入buffer中
		*
		* @param code 待编码的AChar序列
		*
		* @param size code的有效长度
		*
		* @param buffer 编码后的字节串
		*/
		void encode(const AChar* code, c_size size, Buffer& buffer) const
		{
			for (c_size i = 0; i < size; ++i)
			{
				uint32_t unicode = code[i].ord();
				buffer << static_cast<char>(unicode >> 24);
				buffer << static_cast<char>((unicode >> 16) & 0xFF);
				buffer << static_cast<char>((unicode >> 8) & 0xFF);
				buffer << static_cast<char>(unicode & 0xFF);
			}
		}

		/*
		* @brief 将字节串解码为AChar序列, 解码后的AChar序列放入dst中
		*
		* @param dst 解码后的AChar序列
		*
		* @param size dst的有效长度
		*
		* @param bytes 待解码的字节串
		*/
		constexpr void decode(AChar* dst, c_size size, const CString& bytes) const
		{
			const char* bytes_ptr = bytes.data();
			for (c_size i = 0, offset = 0; i < size && offset < bytes.size(); ++i)
			{
				dst[i] = (bytes_ptr[offset] << 24) | (bytes_ptr[offset + 1] << 16) | (bytes_ptr[offset + 2] << 8) | bytes_ptr[offset + 3];
				offset += 4;
			}
		}
	};

	// 根据解码方式，获取解码后的字符串长度
	template<UniCodec Codec>
	constexpr c_size decode_size(const CString& bytes, const Codec& codec)
	{
		const char* bytes_ptr = bytes.data();
		c_size cnt = 0;
		for (c_size offset = 0; offset < bytes.size();)
		{
			int char_size = codec.first_char_size(bytes_ptr + offset);
			if (char_size == -1)
				EncodingError("Invalid Byte Sequence");
			offset += char_size;
			++cnt;
		}
		return cnt;
	}

	void AChar::__repr__(Buffer& buffer) const { UTF8Codec{}.encode(this, 1, buffer); }

#ifdef AYR_UTF8
	using Codec = UTF8Codec;
#elif defined(AYR_UTF16)
	using Codec = UTF16Codec;
#elif defined(AYR_UTF32)
	using Codec = UTF32Codec;
#else
	using Codec = UTF8Codec;
#endif // AYR_UTF8
}
#endif // AYR_BASE_CODEC_UNICODEC_HPP