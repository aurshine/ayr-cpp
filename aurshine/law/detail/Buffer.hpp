#pragma once

#ifndef AYR_LAW_DETAIL_BUFFER_HPP
#define AYR_LAW_DETAIL_BUFFER_HPP

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>

namespace ayr
{
	template<typename T>
	class Buffer : public Object<Buffer<T>>
	{
		using self = Buffer<T>;
		using super = Object<self>;

	public:
		Buffer() : last_(0), capacity_(0), buffer_(nullptr) {}

		Buffer(size_t size) : last_(0), capacity_(size), buffer_(ayr_alloc<T>(capacity_)) {}

		Buffer(const Buffer& other) : last_(other.last_), capacity_(other.capacity_), buffer_(ayr_alloc<T>(capacity_))
		{
			std::memcpy(buffer_, other.buffer_, sizeof(T) * last_);
		}

		Buffer(Buffer&& other) : last_(other.last_), capacity_(other.capacity_), buffer_(other.buffer_)
		{
			other.buffer_ = nullptr;
			other.last_ = 0;
			other.capacity_ = 0;
		}

		Buffer& operator=(const Buffer& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Buffer& operator=(Buffer&& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		~Buffer()
		{
			for (c_size i = 0; i < last_; ++i)
				ayr_destroy(buffer_ + i);
			ayr_delloc(buffer_, capacity_);
		}

		c_size size() const { return last_; }

		c_size capacity() const { return capacity_; }

		T* data() const { return buffer_; }

		// 在buffer末尾追加元素
		template<typename... Args>
		T& append(Args&&... args)
		{
			ayr_construct(buffer_ + last_, std::forward<Args>(args)...);
			return buffer_[last_++];
		}

		void pop_back()
		{
			ayr_destroy(buffer_ + last_);
			--last_;
		}

		// 重新分配内存
		void resize(c_size size)
		{
			ayr_destroy(this);
			ayr_construct(this, size);
		}

		const T& at(c_size index) const { return buffer_[index]; }

		T& at(c_size index) { return buffer_[index]; }

		const T& operator[](c_size index) const { return at(neg_index(index, last_)); }

		T& operator[](c_size index) { return at(neg_index(index, last_)); }
	private:

		c_size last_, capacity_;

		T* buffer_;
	};
}
#endif