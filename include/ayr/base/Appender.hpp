#ifndef AYR_BASE_APPENDER_HPP
#define AYR_BASE_APPENDER_HPP

#include "Array.hpp"

namespace ayr
{
	template<typename T>
	class Appender : public Sequence<Appender<T>, T>
	{
		using self = Appender<T>;

		using super = Sequence<Appender<T>, T>;
	public:
		using Value_t = T;

		using Iterator = super::Iterator;

		using ConstIterator = super::ConstIterator;

		Appender() : size_(0), capacity_(0), buffer_(nullptr) {}

		Appender(c_size size) : size_(0), capacity_(size), buffer_(ayr_alloc<Value_t>(capacity_)) {}

		Appender(const Appender& other) : Appender(other.size())
		{
			for (auto& item : other)
				append(item);
		}

		Appender(Appender&& other) noexcept : size_(other.size_), capacity_(other.capacity_), buffer_(other.buffer_)
		{
			other.buffer_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
		}

		Appender& operator=(const Appender& other)
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		Appender& operator=(Appender&& other) noexcept
		{
			if (this == &other) return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		~Appender()
		{
			ayr_desloc(buffer_, size_);
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

		void pop_back(c_size n = 1)
		{
			while (n--) ayr_destroy(buffer_ + (--size_));
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

		void __swap__(self& other)
		{
			swap(size_, other.size_);
			swap(capacity_, other.capacity_);
			swap(buffer_, other.buffer_);
		}
	private:
		c_size size_, capacity_;

		Value_t* buffer_;
	};
}
#endif