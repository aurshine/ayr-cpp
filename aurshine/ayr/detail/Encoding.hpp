#ifndef AYR_DETAIL_ENCODING
#define AYR_DETAIL_ENCODING

#include <ayr/detail/object.hpp>
#include <ayr/detail/printer.hpp>


namespace ayr
{
	// 单字节
	using Byte = char;


	class Encoding : public Object<Encoding>
	{
		using self = Encoding;

		using super = Object<self>;

	public:
		virtual CString __str__() const { return "Encoding"; }

		// 返回当前编码方式，开头字符的字节数
		virtual int byte_count(const Byte* data) const = 0;

		// 返回当前编码方式的克隆
		virtual Encoding* clone() const = 0;

		virtual ~Encoding() = default;
	};


	class ASCII : public Encoding
	{
		using self = ASCII;

		using super = Encoding;

	public:
		CString __str__() const override { return "ASCII"; }

		int byte_count(const Byte* data) const override { return 1; }

		ASCII* clone() const override { return new ASCII(); }
	};


	class UTF8 : public Encoding
	{
		using self = UTF8;

		using super = Encoding;

	public:
		CString __str__() const override { return "UTF-8"; }

		int byte_count(const Byte* data) const override
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
				ValueError("Invalid CodePoint");
			return 0;
		}

		UTF8* clone() const override { return new UTF8(); }
	};


	class UTF16 : public Encoding
	{
		using self = UTF16;

		using super = Encoding;

	public:
		CString __str__() const  override { return "UTF-16"; }

		int byte_count(const Byte* data) const override
		{
			if ((data[0] & 0xFC) == 0xD8) // 以110110开头 (110110xx 110111xx),4字节编码
				return 4;
			else
				return 2;
		}

		UTF16* clone() const override { return new UTF16(); }
	};


	class UTF32 : public Encoding
	{
		using self = UTF32;

		using super = Encoding;

	public:
		CString __str__() const  override { return "UTF-32"; }

		int byte_count(const Byte* data) const override { return 4; }

		UTF32* clone() const override { return new UTF32(); }
	};


	class GB2312 : public Encoding
	{
		using self = GB2312;

		using super = Encoding;

	public:
		CString __str__() const  override { return "GB2312"; }

		int byte_count(const Byte* data) const override { return 0; }

		GB2312* clone() const override { return new GB2312(); }
	};
}
#endif