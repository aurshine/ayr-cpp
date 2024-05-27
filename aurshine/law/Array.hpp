#pragma once
#include <algorithm>
#include <string>
#include "printer.hpp"
#include "container.hpp"

namespace ayr
{
	template<typename T>
	class Array : public Container<T>
	{
	public:
		Array() : arr_(nullptr), size_(0) {}

		Array(c_size size__) : Array() { alloc(size__); }

		Array(const T& fill_, c_size size__) : Array(size__) { fill(fill_); }

		Array(T* raw_arr, c_size size__) : arr_(raw_arr), size_(size__) {}

		Array(const std::initializer_list<T>& init_list) : Array(init_list.size()) { fill(init_list); }

		Array(const Array& other) : Array(other.size_) { fill(other); }

		Array(Array&& other) : Array(other.arr_, other.size_) { other.arr_ = nullptr, other.size_ = 0; }

		~Array() { release(); }

		Array& operator= (const Array& other)
		{
			if (this == &other) return *this;

			release();
			alloc(other.size_);
			fill(other);
			return *this;
		}

		Array& operator= (Array&& other)
		{
			if (this == &other) return *this;

			release();
			std::swap(arr_, other.arr_);
			std::swap(size_, other.size_);

			return *this;
		}

		c_size size() const { return size_; }

		T* ptr() { return arr_; }

		const T* data() const { return arr_; }

		void fill(const T& fill_val, c_size pos = 0)
		{
			for (c_size i = pos; i < size_; ++i)
				arr_[i] = fill_val;
		}

		void fill(const T* fill_arr, c_size size__, c_size pos = 0)
		{
			c_size min_size = std::min(size__, size_ + pos);
			for (c_size i = pos; i < min_size; ++i)
				arr_[i] = fill_arr[i - pos];
		}

		void fill(const T* fill_arr, c_size size__, const T& default_, c_size pos = 0)
		{
			fill(fill_arr, size__, pos);
			if (size_ - pos - size__ > 0)
				fill(default_, size_ - pos - size_);
		}

		void fill(const Array& other, c_size pos = 0)
		{
			c_size min_size = std::min(size_, other.size_ + pos);
			for (c_size i = pos; i < min_size; ++i)
				arr_[i] = other.arr_[i - pos];
		}

		void fill(const Array& other, const T& default_, c_size pos = 0)
		{
			fill(other, pos);
			if (size_ - pos - other.size_ > 0)
				fill(default_, size_ - pos - other.size_);
		}

		template<typename Iterable>
		void fill(Iterable&& iter)
		{
			c_size idx = 0;
			for (auto it : iter)
			{
				if (idx == size_) break;
				arr_[idx++] = std::forward<T>(it);
			}
		}

		template<typename Iterable>
		void fill(Iterable&& iter, const T& default_)
		{
			c_size idx = 0;
			for (auto it : iter)
			{
				if (idx == size_) break;
				arr_[idx++] = std::forward<T>(it);
			}

			while (idx < size_) arr_[idx++] = default_;
		}

		bool contains(const T& item) const override
		{
			for (c_size i = 0; i < size_; ++i)
				if (arr_[i] == item)
					return true;
			return false;
		}

		T& operator[] (c_size index)
		{
			assert_insize(index);

			index = (index + size_) % size_;
			return arr_[index];
		}

		const T& operator[] (c_size index) const
		{
			assert_insize(index);

			index = (index + size_) % size_;
			return arr_[index];
		}

		// 切片，[l, r)
		Array slice(c_size l, c_size r)
		{
			assert_insize(l), assert_insize(r);

			l = (l + size_) % size_, r = (r + size_) % size_;

			Array<T> ret((r - l + size_) % size_);

			for (c_size i = 0; i < ret.size_; ++i)
				ret.arr_[i] = arr_[(l + i) % size_];

			return ret;
		}

		std::string __str__() const override
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << arr_[i];
			}
			stream << "]";
			return stream.str();
		}

		c_size __cmp__(const Array<T>& other) const
		{
			for (c_size i = 0; i < std::min(size_, other.size_); ++i)
			{
				if (arr_[i] == other.arr_[i])
					continue;

				if (arr_[i] > other.arr_[i])
					return 1;
				else
					return -1;
			}

			return size_ - other.size_;
		}

		// 先释放再分配
		void alloc(c_size size__)
		{
			error_assert(size__ > 0, "size must be greater than 0");
			release();
			arr_ = new T[size__]{};
			size_ = size__;
		}

		// 释放内存
		void release()
		{
			delete[] arr_;
			arr_ = nullptr;
			size_ = 0;
		}

	private:
		void assert_insize(c_size x) const
		{
			error_assert(x >= -size_ && x < size_, std::format("{} out of range [{}, {})", x, -size_, size_));
		};

		T* arr_;

		c_size size_;
	};
}