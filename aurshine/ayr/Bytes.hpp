#ifndef AYRLBYTES_HPP
#define AYRLBYTES_HPP

#include <ayr/detail/Encoding.hpp>
#include <ayr/Dict.hpp>


namespace ayr
{
	Encoding* encoding_map(const CString& encoding_name)
	{
		static Dict<CString, Encoding*> encodingMap_{
			{"ascii", new ASCII()},
			{"utf-8", new UTF8()},
			{"utf-16", new UTF16()},
			{"utf-32", new UTF32()},
			{"gb2312", new GB2312()}
		};

		return encodingMap_.get(encoding_name);
	}


	// 码点
	class CodePoint : public Object<CodePoint>
	{
		using self = CodePoint;

		using super = Object<self>;

	public:
		CodePoint() : byte_code_(nullptr), code_size_(0) {}

		CodePoint(const Byte* bytes, const CString& encoding)
		{
			code_size_ = encoding_map(encoding)->byte_count(bytes);
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
			if (this == &other) return *this;
			code_size_ = other.code_size_;
			byte_code_ = std::make_unique<Byte[]>(code_size_);
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
		const Byte* bytes() const noexcept { return byte_code_.get(); }

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

	private:
		std::unique_ptr<Byte[]> byte_code_;

		int8_t code_size_;
	};
}


#endif 