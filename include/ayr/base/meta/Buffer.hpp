#ifndef AYR_BASE_META_BUFFER_HPP
#define AYR_BASE_META_BUFFER_HPP

#include <cstring>

#include "ayr_memory.hpp"
#include "sprintf.h"

namespace ayr
{
	class Buffer
	{
		char* data_, *read_ptr_, *write_ptr_, *end_ptr_;

		constexpr static c_size INITIAL_CAPACITY = 64;
	public:
		Buffer() : Buffer(INITIAL_CAPACITY) {}

		Buffer(c_size capacity) : 
			data_(ayr_alloc<char>(capacity)), 
			read_ptr_(data_),
			write_ptr_(data_),
			end_ptr_(data_ + capacity) {}

		Buffer(const Buffer& other) : Buffer(other.capacity()) { append_bytes(other.peek(), other.readable_size()); }

		Buffer(Buffer&& other) noexcept : 
			data_(std::exchange(other.data_, nullptr)), 
			read_ptr_(std::exchange(other.read_ptr_, nullptr)),
			write_ptr_(std::exchange(other.write_ptr_, nullptr)),
			end_ptr_(std::exchange(other.end_ptr_, nullptr)) {}

		~Buffer() { ayr_desloc(data_, capacity()); }

		Buffer& operator=(const Buffer& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);

			return *ayr_construct(this, other);
		}

		Buffer& operator=(Buffer&& other) noexcept
		{
			if (this == &other) return *this;
			ayr_destroy(this);

			return *ayr_construct(this, std::move(other));
		}

		// 读缓冲区的大小
		c_size readable_size() const { return write_ptr_ - read_ptr_; }

		// 写缓冲区的大小
		c_size writeable_size() const { return end_ptr_ - write_ptr_; }

		// 缓冲区的容量
		c_size capacity() const { return end_ptr_ - data_; }

		// 清空缓冲区
		void clear() { read_ptr_ = write_ptr_ = data_; }

		// 读缓冲区起始位置
		const char* peek() const { return read_ptr_; }
		/*
		* @brief 从缓冲区中取出指定数量的字节
		* 
		* @param size 要取出的字节数
		* 
		* @note 如果取出的字节数大于缓冲区中可读字节数，则清空缓冲区
		*/
		void retrieve(c_size size)
		{
			if (size >= readable_size())
				clear();
			else
				read_ptr_ += size;
		}

		// 返回 '\n' 位置
		c_size find_eol() { return find("\n"); }

		// 返回 '\r\n' 位置
		c_size find_crlf() { return find("\r\n"); }

		// 返回指定字符串的位置
		c_size find(const char* pattern) 
		{
			c_size pattern_size = std::strlen(pattern);
			for (const char* ptr = read_ptr_; ptr < write_ptr_; ++ptr)
				if (std::memcmp(ptr, pattern, pattern_size) == 0)
					return ptr - read_ptr_;
			return -1;
		}

		// 追加字节
		void append_bytes(const void* bytes, c_size size)
		{
			if (size > writeable_size()) expand_util(size);
			std::memcpy(write_ptr_, bytes, size);
			write_ptr_ += size;
		}

		void __swap__(Buffer& other)
		{
			std::swap(data_, other.data_);
			std::swap(read_ptr_, other.read_ptr_);
			std::swap(write_ptr_, other.write_ptr_);
			std::swap(end_ptr_, other.end_ptr_);
		}

	private:
		/*
		* @brief 扩容写缓冲区到至少min_capacity大小
		* 
		* @param min_capacity 期望最小容量
		*/
		void expand_util(c_size min_write_size)
		{
			c_size capacity = this->capacity() * 2, read_size = readable_size();
			while (capacity < read_size + min_write_size) capacity *= 2;

			char* tmp = ayr_alloc<char>(capacity);
			std::memcpy(tmp, peek(), readable_size());
			ayr_desloc(data_, this->capacity());

			data_ = tmp;
			read_ptr_ = tmp;
			write_ptr_ = tmp + read_size;
			end_ptr_ = tmp + capacity;
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