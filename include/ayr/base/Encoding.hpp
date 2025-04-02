#ifndef AYR_BASE_ENCODING
#define AYR_BASE_ENCODING

#include "raise_error.hpp"


namespace ayr
{
	class Encoding : public Object<Encoding>
	{
		using self = Encoding;

		using super = Object<self>;

	public:
		constexpr Encoding() = default;

		virtual CString __str__() const { return "Encoding"; }

		// 返回当前编码方式，开头字符的字节数
		virtual int byte_size(const char* data) const = 0;

		virtual int to_int(const char* data, int size = -1) const = 0;

		virtual CString from_int(int code) const = 0;

		virtual ~Encoding() = default;
	};


	class ASCIIEncoding : public Encoding
	{
		using self = ASCIIEncoding;

		using super = Encoding;

	public:
		constexpr ASCIIEncoding() = default;

		CString __str__() const override { return "ASCII"; }

		constexpr int byte_size(const char* data) const override { return 1; }

		constexpr int to_int(const char* data, int size = -1) const override { return data[0]; }

		CString from_int(int code) const override { return CString(reinterpret_cast<const char*>(&code), 1); }
	};


	class UTF8Encoding : public Encoding
	{
		using self = UTF8Encoding;

		using super = Encoding;

	public:
		constexpr UTF8Encoding() = default;

		CString __str__() const override { return "UTF8"; }

		constexpr int byte_size(const char* data) const override
		{
			if ((data[0] & 0x80) == 0)         // 以0    开头 1字节编码 (0xxxxxxx)
				return 1;
			else if ((data[0] & 0xE0) == 0xC0) // 以110  开头 2字节编码(110xxxxx 10xxxxxx)
				return  2;
			else if ((data[0] & 0xF0) == 0xE0) // 以1110 开头 3字节编码 (1110xxxx 10xxxxxx 10xxxxxx)
				return 3;
			else if ((data[0] & 0xF8) == 0xF0) // 以11110开头 4字节编码 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx),
				return 4;
			else
				ValueError("Invalid AChar");
			return 0;
		}

		constexpr int to_int(const char* data, int size = -1) const override
		{
			if (size == -1)
				size = byte_size(data);

			switch (size)
			{
			case 1: return data[0];
			case 2: return ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
			case 3: return ((data[0] & 0x0F) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
			case 4: return ((data[0] & 0x07) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
			default: ValueError("Invalid AChar");
			}
			return None<int>;
		}

		CString from_int(int code) const override
		{
			NotImplementedError("UTF8Encoding::from_int is not suppoerted yet");
			return CString();
		}
	};


	class UTF16Encoding : public Encoding
	{
		using self = UTF16Encoding;

		using super = Encoding;

	public:
		constexpr UTF16Encoding() = default;

		CString __str__() const  override { return "UTF16"; }

		constexpr int byte_size(const char* data) const override
		{
			// 检查是否是代理对
			uint16_t first = (data[0] << 8) | data[1];
			if (first >= 0xD800 && first <= 0xDBFF) // 高位代理
			{
				uint16_t second = (data[2] << 8) | data[3];
				if (second >= 0xDC00 && second <= 0xDFFF) // 低位代理
					return 4;
			}
			return 2; // BMP 范围
		}

		constexpr int to_int(const char* data, int size = -1) const override
		{
			if (size == -1)
				size = byte_size(data);

			switch (size)
			{
			case 2:
				// 处理单个 UTF-16 单元
				return (data[0] << 8) | data[1];
			case 4:
				// 处理代理对
				return 0x10000 + (((data[0] << 8 | data[1]) - 0xD800) << 10) + (data[2] << 8 | data[3]) - 0xDC00;
			default: ValueError("Invalid AChar");
			}
			return None<int>;
		}

		CString from_int(int code) const override
		{
			NotImplementedError("UTF16Encoding::from_int is not suppoerted yet");
			return CString();
		}
	};


	class UTF32Encoding : public Encoding
	{
		using self = UTF32Encoding;

		using super = Encoding;

	public:
		constexpr UTF32Encoding() = default;

		CString __str__() const  override { return "UTF32"; }

		constexpr int byte_size(const char* data) const override { return 4; }

		constexpr int to_int(const char* data, int size = -1) const override
		{
			return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		}

		CString from_int(int code) const override
		{
			NotImplementedError("UTF32Encoding::from_int is not suppoerted yet");
			return CString();
		}
	};


	class GB2312Encoding : public Encoding
	{
		using self = GB2312Encoding;

		using super = Encoding;

	public:
		constexpr GB2312Encoding() = default;

		CString __str__() const  override { return "GB2312"; }

		int byte_size(const char* data) const override
		{
			NotImplementedError("GB2312Encoding is not suppoerted yet");
			return 0;
		}

		int to_int(const char* data, int size = -1) const override
		{
			NotImplementedError("GB2312Encoding is not suppoerted yet");
			return 0;
		}

		CString from_int(int code) const override
		{
			NotImplementedError("GB2312Encoding::from_int is not suppoerted yet");
			return CString();
		}
	};

	static std::unique_ptr<Encoding> _u_ascii = std::make_unique<ASCIIEncoding>();
	static std::unique_ptr<Encoding> _u_utf8 = std::make_unique<UTF8Encoding>();
	static std::unique_ptr<Encoding> _u_utf16 = std::make_unique<UTF16Encoding>();
	static std::unique_ptr<Encoding> _u_utf32 = std::make_unique<UTF32Encoding>();
	static std::unique_ptr<Encoding> _u_gb2312 = std::make_unique<GB2312Encoding>();

	static Encoding* ASCII = _u_ascii.get();
	static Encoding* UTF8 = _u_utf8.get();
	static Encoding* UTF16 = _u_utf16.get();
	static Encoding* UTF32 = _u_utf32.get();
	static Encoding* GB2312 = _u_gb2312.get();
} // namespace ayr
#endif