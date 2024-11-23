#pragma once

#ifndef AYR_DEValue_tAIL_BUFFER_HPP
#define AYR_DEValue_tAIL_BUFFER_HPP

#include <ayr/base/printer.hpp>
#include <ayr/base/ayr_memory.hpp>
#include <ayr/base/Array.hpp>
#include <ayr/base/RelationIterator.hpp>

namespace ayr
{
	template<typename T>
	class Buffer : public Sequence<Buffer<T>, T>
	{
		using self = Buffer<T>;

		using super = Sequence<Buffer<T>, T>;

	public:
		using Value_t = T;

		using Iterator = super::Iterator;

		using ConstIterator = super::ConstIterator;

		Buffer() : size_(0), capacity_(0), buffer_(nullptr) {}

		Buffer(c_size size) : size_(0), capacity_(size), buffer_(ayr_alloc<Value_t>(capacity_)) {}

		Buffer(const Buffer& other) : size_(other.size_), capacity_(other.capacity_), buffer_(ayr_alloc<Value_t>(capacity_))
		{
			for (c_size i = 0; i < other.size(); ++i)
				ayr_construct(buffer_ + i, *(other.buffer_ + i));
		}

		Buffer(Buffer&& other) : size_(other.size_), capacity_(other.capacity_), buffer_(other.buffer_)
		{
			other.buffer_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
		}

		Buffer& operator=(const Buffer& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Buffer& operator=(Buffer&& other) noexcept
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		~Buffer()
		{
			ayr_destroy(buffer_, size());
			ayr_delloc(buffer_);
			size_ = capacity_ = 0;
		}

		c_size size() const { return size_; }

		c_size capacity() const { return capacity_; }

		bool full() const { return size_ == capacity_; }

		Value_t* data() { return buffer_; }

		const Value_t* data() const { return buffer_; }

		// 在buffer末尾追加元素
		template<typename... Args>
		Value_t& append(Args&&... args)
		{
			ayr_construct(buffer_ + size_, std::forward<Args>(args)...);
			return buffer_[size_++];
		}

		void append_bytes(const void* ptr, c_size size)
		{
			std::memcpy(buffer_ + size_, ptr, size);
			size_ += size;
		}

		void pop_back()
		{
			ayr_destroy(buffer_ + size_);
			--size_;
		}

		// 重新分配内存
		void resize(c_size size)
		{
			ayr_destroy(this);
			ayr_construct(this, size);
		}

		const Value_t& at(c_size index) const { return buffer_[index]; }

		Value_t& at(c_size index) { return buffer_[index]; }

		Array<Value_t> to_array() const
		{
			Array<Value_t> arr(size());
			for (c_size i = 0; i < size(); ++i)
				arr[i] = at(i);
			return arr;
		}

		Array<Value_t> move_array()
		{
			Array<Value_t> arr(size());
			for (c_size i = 0; i < size(); ++i)
				arr[i] = std::move(at(i));

			size_ = 0;
			return arr;
		}
	private:

		c_size size_, capacity_;

		Value_t* buffer_;
	};
}
#endif