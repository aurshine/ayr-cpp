#pragma once

#ifndef AYR_BASE_BUFFER_HPP
#define AYR_BASE_BUFFER_HPP

#include "Appender.hpp"

namespace ayr
{
	class Buffer : public Sequence<Buffer, char>
	{
		using self = Buffer;

		using super = Sequence<Buffer, char>;

	public:
		using Value_t = char;

		using Iterator = super::Iterator;

		using ConstIterator = super::ConstIterator;

		Buffer() : size_(0), capacity_(0), buffer_(nullptr) {}

		Buffer(c_size size) : size_(0), capacity_(size), buffer_(ayr_alloc<Value_t>(capacity_)) {}

		Buffer(const Buffer& other) : size_(other.size_), capacity_(other.capacity_), buffer_(ayr_alloc<Value_t>(capacity_))
		{
			std::memcpy(buffer_, other.buffer_, size_);
		}

		Buffer(Buffer&& other) noexcept : size_(other.size_), capacity_(other.capacity_), buffer_(other.buffer_)
		{
			other.buffer_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
		}

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

		~Buffer()
		{
			ayr_delloc(buffer_);
			size_ = capacity_ = 0;
		}

		c_size size() const { return size_; }

		c_size capacity() const { return capacity_; }

		bool full() const { return size_ == capacity_; }

		Value_t* data() { return buffer_; }

		const Value_t* data() const { return buffer_; }

		const Value_t& at(c_size index) const { return buffer_[index]; }

		void append_bytes(const void* ptr, c_size size)
		{
			std::memcpy(buffer_ + size_, ptr, size);
			size_ += size;
		}

		void append_bytes(const self& other) { append_bytes(other.data(), other.size()); }

	private:
		c_size size_, capacity_;

		Value_t* buffer_;
	};
}
#endif