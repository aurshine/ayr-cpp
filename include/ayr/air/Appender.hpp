#ifndef AYR_AIR_APPENDER_HPP
#define AYR_AIR_APPENDER_HPP

#include "../base.hpp"

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

		Appender(const Appender& other) : Appender(other.capacity())
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
			Value_t* result = ayr_construct(buffer_ + size_, std::forward<Args>(args)...);
			++size_;
			return *result;
		}

		void pop_back(c_size n = 1)
		{
			if (n <= 0) return;

			c_size new_size = std::max<c_size>(0, size_ - n);
			if constexpr (NoDestroy<Value_t>)
				size_ = new_size;
			else
				while (size_ > new_size)
					ayr_destroy(buffer_ + (--size_));
		}

		// 清空元素，保留已分配的内存
		void clear()
		{
			ayr_destroy(buffer_, size_);
			size_ = 0;
		}

		// 重新分配内存
		void resize(c_size size)
		{
			clear();
			if (capacity_ == size)
				return;

			Value_t* new_buffer = ayr_alloc<Value_t>(size);
			ayr_delloc(buffer_);
			buffer_ = new_buffer;
			capacity_ = size;
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

			clear();
			return arr;
		}

		std::strong_ordering operator<=>(const self& other) const { return super::operator<=>(other); }

		bool operator==(const self& other) const { return super::operator==(other); }
	private:
		c_size size_, capacity_;

		Value_t* buffer_;
	};
}
#endif // AYR_AIR_APPENDER_HPP