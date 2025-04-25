#ifndef AYR_BASE_META_BUFFER_HPP
#define AYR_BASE_META_BUFFER_HPP

#include "ayr_memory.hpp"
#include "sprintf.h"

namespace ayr 
{
	class Buffer
	{
		char* data_;
		c_size size_, capacity_;

	public:
		Buffer(): Buffer(128) {}

		Buffer(c_size capacity) : data_(ayr_alloc<char>(capacity)), size_(0), capacity_(capacity) {}
		
		Buffer(const Buffer& other) : Buffer(other.capacity_)
		{
			std::memcpy(data_, other.data_, other.size_);
			size_ = other.size_;
		}

		Buffer(Buffer&& other) noexcept: data_(other.data_), size_(other.size_), capacity_(other.capacity_)
		{
			other.data_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
		}

		~Buffer() { ayr_desloc(data_, size_); }

		Buffer& operator=(Buffer&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_desloc(data_, size_);
			data_ = other.data_;
			size_ = other.size_;
			capacity_ = other.capacity_;
			other.data_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
			return *this;
		}

		c_size size() const { return size_; }

		c_size capacity() const { return capacity_; }

		c_size clear() { return size_ = 0; }

		const char* data() const { return data_; }

		void append_bytes(const void* bytes, c_size size)
		{
			if (size + size_ >= capacity_)
				expand(size + size_);
			std::memcpy(data_ + size_, bytes, size);
			size_ += size;
			data_[size_] = '\0';
		}

		def repr_int(c_size value)
		{
			char tmp[32];
			sprintf_int(tmp, sizeof(tmp), value);
			append_bytes(tmp, strlen(tmp));
		}

		def repr_float(double value)
		{
			char tmp[32];
			sprintf_float(tmp, sizeof(tmp), value);
			append_bytes(tmp, strlen(tmp));
		}

		def repr_pointer(const void* value)
		{
			char tmp[32];
			sprintf_pointer(tmp, sizeof(tmp), value);
			append_bytes(tmp, strlen(tmp));
		}

		def repr(bool value)
		{
			if (value)
				append_bytes("true", 4);
			else
				append_bytes("false", 5);
		}

		def repr(char value)
		{
			append_bytes(&value, 1);
		}

		def repr(const char* value)
		{
			append_bytes(value, std::strlen(value));
		}

		def repr(std::nullptr_t value)
		{
			append_bytes("nullptr", 7);
		}

		def repr(const std::string& value)
		{
			append_bytes(value.data(), value.size());
		}

		def repr(const std::string_view& value)
		{
			append_bytes(value.data(), value.size());
		}

		template<typename T>
		def meta_repr(const T& value)
		{
			*this << "<" << dtype(T) << " 0x" << &value << ">";
		}

		template<typename T>
		def repr(const T& value)
		{
			if constexpr (hasmethod(T, __repr__, *this))
				value.__repr__(*this);
			else if constexpr (hasmethod(T, __str__))
			{
				CString str = value.__str__();
				append_bytes(str.data(), str.size());
			}
			else if constexpr (std::is_integral_v<T>)
				repr_int(value);
			else if constexpr (std::is_floating_point_v<T>)
				repr_float(value);
			else if constexpr (std::is_pointer_v<T>)
				repr_pointer(value);
			else
				meta_repr(value);
		}

		template<typename T>
		Buffer& operator<< (const T& value) { repr(value); return *this; }

		void __swap__(Buffer& other)
		{
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
		}
	private:
		void expand(c_size min_capacity)
		{
			while (capacity_ <= min_capacity)
				capacity_ = ifelse(capacity_ <= 128, 128, capacity_ * 2);

			char* tmp = ayr_alloc<char>(capacity_);
			std::memcpy(tmp, data_, size_);
			ayr_desloc(data_, size_);
			data_ = tmp;
		}
	};
}
#endif // AYR_BASE_META_BUFFER_HPP