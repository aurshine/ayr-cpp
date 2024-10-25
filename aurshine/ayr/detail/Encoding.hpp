#ifndef AYR_DETAIL_ENCODING
#define AYR_DETAIL_ENCODING

#include <ayr/detail/object.hpp>
#include <ayr/detail/printer.hpp>


namespace ayr
{
	const CString ASCII = "ascii";
	const CString UTF8 = "utf-8";
	const CString UTF16 = "utf-16";
	const CString UTF32 = "utf-32";
	const CString GB2312 = "gb2312";

	class Encoding : public Object<Encoding>
	{
		using self = Encoding;

		using super = Object<self>;

	public:
		virtual CString __str__() const { return "Encoding"; }

		// 返回当前编码方式，开头字符的字节数
		virtual int byte_size(const char* data) const = 0;

		// 返回当前编码方式的克隆
		virtual Encoding* clone() const = 0;

		virtual ~Encoding() = default;

		virtual c_size bytes_count(const char* data, c_size len)
		{
			c_size count = 0;
			int i = 0;
			while (i < len)
			{
				++count;
				int byte_size_ = this->byte_size(data + i);
				i += byte_size_;
				if (i > len) EncodingError("Invalid encoding");
			}
			return count;
		}
	};


	class ASCIIEncoding : public Encoding
	{
		using self = ASCIIEncoding;

		using super = Encoding;

	public:
		CString __str__() const override { return "ASCII"; }

		int byte_size(const char* data) const override { return 1; }

		self* clone() const override { return new self(); }
	};


	class UTF8Encoding : public Encoding
	{
		using self = UTF8Encoding;

		using super = Encoding;

	public:
		CString __str__() const override { return "UTF-8"; }

		int byte_size(const char* data) const override
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

		self* clone() const override { return new self(); }
	};


	class UTF16Encoding : public Encoding
	{
		using self = UTF16Encoding;

		using super = Encoding;

	public:
		CString __str__() const  override { return "UTF-16"; }

		int byte_size(const char* data) const override
		{
			if ((data[0] & 0xFC) == 0xD8) // 以110110开头 (110110xx 110111xx),4字节编码
				return 4;
			else
				return 2;
		}

		self* clone() const override { return new self(); }
	};


	class UTF32Encoding : public Encoding
	{
		using self = UTF32Encoding;

		using super = Encoding;

	public:
		CString __str__() const  override { return "UTF-32"; }

		int byte_size(const char* data) const override { return 4; }

		self* clone() const override { return new self(); }
	};


	class GB2312Encoding : public Encoding
	{
		using self = GB2312Encoding;

		using super = Encoding;

	public:
		CString __str__() const  override { return "GB2312"; }

		int byte_size(const char* data) const override { return 0; }

		self* clone() const override { return new self(); }
	};
}
#endif