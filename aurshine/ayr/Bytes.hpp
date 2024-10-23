#ifndef AYR_LAWLBYTES_HPP
#define AYR_LAWLBYTES_HPP

#include <cstdint>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/ayr_memory.hpp>

namespace ayr
{
	using Byte = char;
	static_assert(sizeof(Byte) == 1, "Byte must be 1 byte");


	class Encodings : public Object<Encodings>
	{
		using self = Encodings;

		using super = Object<self>;

	public:
		virtual CString __str__() const { return "Encoding"; }

		// 返回当前编码方式，开头字符的字节数
		virtual int byte_count(const Byte* data) const = 0;

		// 返回当前编码方式的克隆
		virtual Encodings* clone() const = 0;

		virtual ~Encodings() = default;
	};


	class ASCII : public Encodings
	{
		using self = ASCII;

		using super = Encodings;

	public:
		virtual CString __str__() const { return "ASCII"; }

		virtual int byte_count(const Byte* data) const { return 1; }

		virtual ASCII* clone() const override { return new ASCII(); }
	};


	class UTF8 : public Encodings
	{
		using self = UTF8;

		using super = Encodings;

	public:
		virtual CString __str__() const override { return "UTF-8"; }

		virtual int byte_count(const Byte* data) const override
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

		virtual UTF8* clone() const override { return new UTF8(); }
	};


	class UTF16 : public Encodings
	{
		using self = UTF16;

		using super = Encodings;

	public:
		virtual CString __str__() const { return "UTF-16"; }

		virtual int byte_count(const Byte* data) const
		{
			if ((data[0] & 0xFC) == 0xD8) // 以110110开头 (110110xx 110111xx),4字节编码
				return 4;
			else
				return 2;
		}

		virtual UTF16* clone() const override { return new UTF16(); }
	};


	class UTF32 : public Encodings
	{
		using self = UTF32;

		using super = Encodings;

	public:
		virtual CString __str__() const { return "UTF-32"; }

		virtual int byte_count(const Byte* data) const { return 4; }

		virtual UTF32* clone() const override { return new UTF32(); }
	};



	class GB2312 : public Encodings
	{
		using self = GB2312;

		using super = Encodings;

	public:
		virtual CString __str__() const { return "GB2312"; }

		virtual int byte_count(const Byte* data) const { return 0; }

		virtual GB2312* clone() const override { return new GB2312(); }
	};


	// 码点
	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<self>;

	public:
		CodePoint(const Byte* bytes, const Encodings* encoding)
		{
			code_size_ = encoding->byte_count(bytes);
			byte_code_ = std::make_unique<Byte[]>(code_size_);
			std::memcpy(byte_code_.get(), bytes, code_size_);
		}

		CodePoint(const self& other)
		{
			code_size_ = other.size();
			byte_code_ = std::make_unique<Byte[]>(code_size_);
			std::memcpy(byte_code_.get(), other.bytes(), code_size_);
		}

		CodePoint(self&& other) noexcept : byte_code_(std::move(other.byte_code_)), code_size_(other.code_size_) {}

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

		// 返回字节码
		const Byte* bytes() const noexcept { return byte_code_.get(); }

		// 返回字节码长度
		c_size size() const noexcept { return code_size_; }

		CString __str__() const override { return CString(reinterpret_cast<const char*>(bytes()), size()); }

		hash_t __hash__() const override { return bytes_hash(reinterpret_cast<const char*>(bytes()), size()); }

		cmp_t __cmp__(const self& other)
		{
			c_size m_size = size(), o_size = other.size();
			if (m_size != o_size)
				return m_size - o_size;

			return std::memcmp(bytes(), other.bytes(), m_size);
		}

	private:
		std::unique_ptr<Byte[]> byte_code_;

		int8_t code_size_;
	};
}


#endif 