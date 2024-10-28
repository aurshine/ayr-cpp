#pragma once

#ifndef AYR_DETAIL_BUFFER_HPP
#define AYR_DETAIL_BUFFER_HPP

#include <ayr/detail/printer.hpp>
#include <ayr/detail/ayr_memory.hpp>
#include <ayr/detail/Array.hpp>
#include <ayr/detail/RelationIterator.hpp>

namespace ayr
{
	template<typename T>
	class Buffer : public Object<Buffer<T>>
	{
		using self = Buffer<T>;

		using super = Object<self>;

	public:
		using Iterator = RelationIterator<SelfAddMove<T*>>;

		using ConstIterator = RelationIterator<SelfAddMove<const T*>>;

		Buffer() : size_(0), capacity_(0), buffer_(nullptr) {}

		Buffer(size_t size) : size_(0), capacity_(size), buffer_(ayr_alloc<T>(capacity_)) {}

		Buffer(const Buffer& other) : size_(other.size_), capacity_(other.capacity_), buffer_(ayr_alloc<T>(capacity_))
		{
			for (c_size i = 0; i < other.size(); ++i)
				ayr_construct(buffer_ + i, other.buffer_ + i);
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

		Buffer& operator=(Buffer&& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		~Buffer()
		{
			for (c_size i = 0; i < size(); ++i)
				ayr_destroy(buffer_ + i);
			ayr_delloc(buffer_);
		}

		c_size size() const { return size_; }

		c_size capacity() const { return capacity_; }

		T* data() { return buffer_; }

		const T* data() const { return buffer_; }

		// 在buffer末尾追加元素
		template<typename... Args>
		T& append(Args&&... args)
		{
			ayr_construct(buffer_ + size_, std::forward<Args>(args)...);
			return buffer_[size_++];
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

		const T& at(c_size index) const { return buffer_[index]; }

		T& at(c_size index) { return buffer_[index]; }

		const T& operator[](c_size index) const { return at(neg_index(index, size_)); }

		T& operator[](c_size index) { return at(neg_index(index, size_)); }

		Array<T> to_array() const { return self(*this).move_array(); }

		Array<T> move_array()
		{
			Array<T> arr(data(), size());
			size_ = capacity_ = 0;
			buffer_ = nullptr;
			return arr;
		}

		Iterator begin() { return Iterator(buffer_); }

		ConstIterator begin() const { return ConstIterator(buffer_); }

		Iterator end() { return Iterator(buffer_ + size_); }

		ConstIterator end() const { return ConstIterator(buffer_ + size_); }
	private:

		c_size size_, capacity_;

		T* buffer_;
	};
}
#endif