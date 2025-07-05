#ifndef AYR_BASE_META_BUFFER_HPP
#define AYR_BASE_META_BUFFER_HPP

#include <cstring>

#include "ayr_memory.hpp"
#include "sprintf.h"

namespace ayr
{
	class Buffer
	{
		char* data_;

		c_size size_, capacity_;

		constexpr static c_size INITIAL_CAPACITY = 64;
	public:
		Buffer() : Buffer(INITIAL_CAPACITY) {}

		Buffer(c_size capacity) : data_(ayr_alloc<char>(capacity + 1)), size_(0), capacity_(capacity) {}

		Buffer(const Buffer& other) : Buffer(other.capacity_)
		{
			std::memcpy(data_, other.data_, other.size_);
			size_ = other.size_;
		}

		Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
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
			if (size + size_ > capacity_)
				expand(size + size_);
			std::memcpy(data_ + size_, bytes, size);
			size_ += size;
			data_[size_] = '\0';
		}

		void __swap__(Buffer& other)
		{
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
		}

		void expand(c_size min_capacity)
		{
			while (capacity_ < min_capacity)
				capacity_ = ifelse(capacity_ < INITIAL_CAPACITY, INITIAL_CAPACITY, capacity_ * 2);

			char* tmp = ayr_alloc<char>(capacity_);
			std::memcpy(tmp, data_, size_);
			ayr_desloc(data_, size_);
			data_ = tmp;
		}
	};

	void _repr_int(Buffer& buffer, c_size value)
	{
		char tmp[32];
		sprintf_int(tmp, sizeof(tmp), value);
		buffer.append_bytes(tmp, strlen(tmp));
	}

	void _repr_float(Buffer& buffer, double value)
	{
		char tmp[32];
		sprintf_float(tmp, sizeof(tmp), value);
		buffer.append_bytes(tmp, strlen(tmp));
	}

	void _repr_pointer(Buffer& buffer, const void* value)
	{
		char tmp[32];
		sprintf_pointer(tmp, sizeof(tmp), value);
		buffer.append_bytes(tmp, strlen(tmp));
	}

	Buffer& operator<<(Buffer& buffer, bool value)
	{
		if (value)
			buffer.append_bytes("true", 4);
		else
			buffer.append_bytes("false", 5);
		return buffer;
	}

	Buffer& operator<<(Buffer& buffer, char value)
	{
		buffer.append_bytes(&value, 1);
		return buffer;
	}

	Buffer& operator<<(Buffer& buffer, const char* value)
	{
		buffer.append_bytes(value, std::strlen(value));
		return buffer;
	}

	Buffer& operator<<(Buffer& buffer, std::nullptr_t value)
	{
		buffer.append_bytes("nullptr", 7);
		return buffer;
	}

	Buffer& operator<<(Buffer& buffer, const std::string& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}

	Buffer& operator<<(Buffer& buffer, const std::string_view& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}

	template<typename T1, typename T2>
	Buffer& operator<< (Buffer& buffer, const std::pair<T1, T2>& value)
	{
		buffer << "(" << value.first << ", " << value.second << ")";
		return buffer;
	}

	template<typename... Args>
	Buffer& operator<< (Buffer& buffer, const std::tuple<Args...>& value)
	{
		buffer << "(";
		std::apply([&](auto&& t1, auto&&... args) {
			buffer << t1;
			if constexpr (sizeof...(args) > 0)
				((buffer << ", " << args), ...);
			}, value);
		buffer << ")";
		return buffer;
	}

	template<typename T>
	Buffer& operator<< (Buffer& buffer, const T& value)
	{
		if constexpr (hasmethod(T, __repr__, std::declval<Buffer&>()))
			value.__repr__(buffer);
		else if constexpr (hasmethod(T, __str__))
		{
			auto str = value.__str__();
			buffer.append_bytes(str.data(), str.size());
		}
		else if constexpr (std::is_integral_v<T>)
			_repr_int(buffer, value);
		else if constexpr (std::is_floating_point_v<T>)
			_repr_float(buffer, value);
		else if constexpr (std::is_pointer_v<T>)
			_repr_pointer(buffer, value);
		else
			buffer << "<" << dtype(T) << " 0x" << &value << ">";
		return buffer;
	}
}
#endif // AYR_BASE_META_BUFFER_HPP