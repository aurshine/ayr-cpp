#ifndef AYR_BASE_META_BUFFER_HPP
#define AYR_BASE_META_BUFFER_HPP

#include <cstring>
#include <string>
#include <utility>

#include "ayr_memory.hpp"
#include "sprintf.h"

namespace ayr
{
	class Buffer
	{
		char* data_, * read_ptr_, * write_ptr_, * end_ptr_;

		constexpr static c_size INITIAL_CAPACITY = 64;
	public:
		friend CString from_buffer(Buffer&& buffer);

		Buffer() : Buffer(INITIAL_CAPACITY) {}

		Buffer(c_size capacity) :
			data_(ayr_alloc<char>(capacity)),
			read_ptr_(data_),
			write_ptr_(data_),
			end_ptr_(data_ + capacity) {
		}

		Buffer(const Buffer& other) : Buffer(other.readable_size()) { append_bytes(other.peek(), other.readable_size()); }

		Buffer(Buffer&& other) noexcept :
			data_(std::exchange(other.data_, nullptr)),
			read_ptr_(std::exchange(other.read_ptr_, nullptr)),
			write_ptr_(std::exchange(other.write_ptr_, nullptr)),
			end_ptr_(std::exchange(other.end_ptr_, nullptr)) {
		}

		~Buffer() { ayr_desloc(data_); }

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

		// 关闭写缓冲区，只能读取数据，不能写入数据
		void close() { end_ptr_ = nullptr; }

		// 判断缓冲区是否被关闭
		bool closed() const { return end_ptr_ == nullptr; }

		// 清空读缓冲区
		void clear() { read_ptr_ = write_ptr_ = data_; }

		// 读缓冲区的大小
		c_size readable_size() const { return write_ptr_ - read_ptr_; }

		/*
		* @brief 写缓冲区的大小
		* 
		* @return 写缓冲区的大小，如果缓冲区被关闭则返回 -1
		*/
		c_size writeable_size() const { return ifelse(closed(), -1, end_ptr_ - write_ptr_); }

		/*
		* @brief 缓冲区的总容量
		* 
		* @return 缓冲区的总容量，如果缓冲区被关闭则返回 -1
		*/
		c_size capacity() const { return ifelse(closed(), -1, end_ptr_ - data_); }

		// 读缓冲区起始位置
		const char* peek() const { return read_ptr_; }

		// 写缓冲区起始位置
		char* write_ptr() const { return write_ptr_; }

		// 缓冲区首地址
		char* data() const { return data_; }

		/*
		* @brief 向写缓冲区中写入指定数量的字节
		* 
		* @param size 要写入的字节数
		* 
		* @note 如果写入的字节数大于缓冲区中可写字节数，则只写入到缓冲区末尾
		*/
		void written(c_size size)
		{
			if (size <= 0) return;
			if (size <= writeable_size())
				write_ptr_ += size;
			else
				write_ptr_ = end_ptr_;
		}

		/*
		* @brief 从读缓冲区中取出指定数量的字节
		*
		* @param size 要取出的字节数
		*
		* @note 如果取出的字节数大于缓冲区中可读字节数，则清空缓冲区
		*/
		void retrieve(c_size size)
		{
			if (size <= 0)
				return;
			if (size >= readable_size())
				clear();
			else
				read_ptr_ += size;
		}

		// 返回读缓冲区中 '\n' 位置
		c_size find_eol(c_size pos = 0) { return find('\n', pos); }

		// 返回读缓冲区中 '\r\n' 位置
		c_size find_crlf(c_size pos = 0) { return find("\r\n", pos); }

		/*
		* @brief 寻找读缓冲区中指定字符的位置
		*
		* @param c 要查找的字符
		*
		* @param pos 起始位置
		*
		* @return 字符位置，如果没有找到则返回 -1
		*/
		c_size find(char c, c_size pos = 0) const
		{
			pos = ifelse(pos < 0, 0, pos);
			for (const char* ptr = read_ptr_ + pos; ptr < write_ptr_; ++ptr)
				if (*ptr == c)
					return ptr - read_ptr_;
			return -1;
		}

		/*
		* @brief 寻找读缓冲区中指定字符串的位置
		*
		* @param pattern 要查找的字符串
		*
		* @param pos 起始位置
		*
		* @return 字符串位置，如果没有找到则返回 -1
		*/
		c_size find(const char* pattern, c_size pos = 0) const
		{
			pos = ifelse(pos < 0, 0, pos);
			c_size pattern_size = std::strlen(pattern);
			for (const char* ptr = read_ptr_ + pos; ptr + pattern_size < write_ptr_; ++ptr)
				if (std::memcmp(ptr, pattern, pattern_size) == 0)
					return ptr - read_ptr_;
			return -1;
		}

		/*
		* @brief 向写缓冲区追加字节
		* 
		* @details 如果追加的字节数超过写缓冲区的可写字节数，则自动扩容写缓冲区到至少size * n大小
		* 
		* @param bytes 要追加的字节指针
		* 
		* @param size 追加字节的大小
		* 
		* @param n 追加字节的数量
		* 
		* @note 如果写缓冲区被关闭，则不执行任何操作
		*/
		void append_bytes(const void* bytes, c_size size, c_size n=1)
		{
			if (closed()) return;
			adjust_util(size * n);
			while (n --)
			{
				std::memcpy(write_ptr_, bytes, size);
				written(size);
			}
		}

		/*
		* @brief 调整可写的写缓冲区到至少min_write_size大小
		*
		* @param min_write_size 写缓冲区期望最小可写容量
		* 
		* @note 如果当前可写容量已经大于等于min_write_size或写缓冲区被关闭，则不执行任何操作
		*/
		void adjust_util(c_size min_write_size)
		{
			if (closed() || min_write_size <= writeable_size())
				return;

			// 如果当前容量足够容纳现有数据和新数据，则将现有数据移动到缓冲区起始位置
			if (capacity() >= readable_size() + min_write_size)
			{
				std::memmove(data_, peek(), readable_size());
				read_ptr_ = data_;
				write_ptr_ = data_ + readable_size();
			}
			else
			{
				c_size capacity_ = capacity() * 2, read_size = readable_size();
				while (capacity_ < read_size + min_write_size) capacity_ *= 2;

				char* tmp = ayr_alloc<char>(capacity_);
				std::memcpy(tmp, peek(), readable_size());
				ayr_desloc(data_);

				data_ = read_ptr_ = tmp;
				write_ptr_ = tmp + read_size;
				end_ptr_ = tmp + capacity_;
			}
		}
	private:
		/*
		* @brief 分离缓冲区底部数据
		*
		* @return 底部数据指针和缓冲区总大小
		*/
		void detach() { data_ = write_ptr_ = read_ptr_ = end_ptr_ = nullptr; }
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