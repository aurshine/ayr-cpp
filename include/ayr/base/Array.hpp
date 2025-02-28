#ifndef AYR_BASE_ARRAY_HPP
#define AYR_BASE_ARRAY_HPP

#include <algorithm>

#include "raise_error.hpp"
#include "ayr_memory.hpp"
#include "Sequence.hpp"


namespace ayr
{
	template<typename T>
	class Array : public Sequence<Array<T>, T>
	{
		using self = Array<T>;

		using super = Sequence<self, T>;
	public:
		using Value_t = T;

		template<typename ...Args>
		Array(c_size size, const Args&... args) : size_(size), arr_(ayr_alloc<T>(size))
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i, args...);
		}

		Array(std::initializer_list<T>&& init_list) : size_(init_list.size()), arr_(ayr_alloc<T>(init_list.size()))
		{
			c_size i = 0;
			for (const T& item : init_list)
				ayr_construct(data() + i++, item);
		}

		Array(const self& other) : size_(other.size_), arr_(ayr_alloc<T>(other.size_))
		{
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(data() + i, other.data()[i]);
		}

		Array(self&& other) noexcept : size_(other.size_), arr_(other.arr_) { other.size_ = 0; other.arr_ = nullptr; }

		~Array()
		{
			ayr_destroy(arr_, size_);
			ayr_delloc(arr_);
			size_ = 0;
		};

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(arr_, size_);
			ayr_delloc(arr_);

			return *ayr_construct(this, other);;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other)
				return *this;

			ayr_destroy(arr_, size_);
			ayr_delloc(arr_);

			size_ = other.size_;
			arr_ = std::move(other.arr_);

			other.size_ = 0;
			other.arr_ = 0;

			return *this;
		}

		T* data() { return arr_; }

		const T* data() const { return arr_; }

		T& at(c_size index) { return arr_[index]; }

		const T& at(c_size index) const { return arr_[index]; }

		T& front() { return *arr_; }

		const T& front() const { return *arr_; }

		T& back() { return arr_[size_ - 1]; }

		const T& back() const { return arr_[size_ - 1]; }

		c_size size() const { return size_; }

		CString __str__() const
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size(); ++i)
			{
				if (i != 0) stream << ", ";
				stream << at(i);
			}
			stream << "]";
			return stream.str();
		}

		// 重新分配内存，不保留原有数据
		template<typename ...Args>
		void resize(c_size new_size, const Args&... args)
		{
			ayr_destroy(this);
			ayr_construct(this, new_size, args...);
		}

		// 分离数组，返回数组指针和大小，并将数组置空
		std::pair<T*, c_size> separate()
		{
			std::pair<T*, c_size> result = { data(), size_ };

			arr_ = nullptr;
			size_ = 0;
			return result;
		}
	private:
		T* arr_;

		c_size size_;
	};
}
#endif