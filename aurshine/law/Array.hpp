#pragma once
#include <algorithm>
#include <functional>
#include <string>

#include "printer.hpp"
#include "container.hpp"

namespace ayr
{
	constexpr static c_size MAX_ALLOC = LLONG_MAX;


	// 检查 x 在[l, r] 区间
	void assert_insize(c_size x, c_size lbound, c_size rbound)
	{
		error_assert(x >= lbound && x <= rbound, std::format("{} out of range [{}, {}]", x, lbound, rbound));
	};
	

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
			swap(other);

			return *this;
		}

		void swap(Array& other)
		{
			std::swap(arr_, other.arr_);
			std::swap(size_, other.size_);
		}

		// fill_val 填充值
		void fill(const T& fill_val, c_size pos = 0)
		{
			assert_insize(pos, 0, size_);
			for (c_size i = pos; i < size_; ++i)
				arr_[i] = fill_val;
		}

		// 以指针填充size__个元素
		void fill(const T* fill_arr, c_size size__, c_size pos = 0)
		{
			assert_insize(pos, 0, size_);
			c_size min_size = std::min(size__, size_ + pos);
			for (c_size i = pos; i < min_size; ++i)
				arr_[i] = fill_arr[i - pos];
		}

		// 以指针填充size__个元素, 未填满使用默认值
		void fill(const T* fill_arr, c_size size__, const T& default_, c_size pos = 0)
		{
			fill(fill_arr, size__, pos);
			if (size_ - pos - size__ > 0)
				fill(default_, size_ - pos - size__);
		}

		// 以Array对象填充元素
		void fill(const Array& other, c_size pos = 0)
		{
			assert_insize(pos, 0, size_);
			c_size min_size = std::min(size_, other.size_ + pos);
			for (c_size i = pos; i < min_size; ++i)
				arr_[i] = other.arr_[i - pos];
		}

		// 以Array对象填充元素, 未填满使用默认值
		void fill(const Array& other, const T& default_, c_size pos = 0)
		{
			fill(other, pos);
			if (size_ - pos - other.size_ > 0)
				fill(default_, size_ - pos - other.size_);
		}

		// 以迭代器填充元素
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

		// 以迭代器填充元素, 未填满使用默认值
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

		// 容器大小
		c_size size() const { return size_; }

		// 容器底层指针
		T* ptr() { return arr_; }

		// 容器底层指针
		const T* ptr() const { return arr_; }

		// 元素是否存在
		bool contains(const T& item) const override { return find(item) != -1; }

		// 寻找元素返回下标
		c_size find(const T& find_item, c_size pos = 0) const
		{
			assert_insize(pos, 0, size_ - 1);

			while (pos < size_ && arr_[pos] != find_item) ++pos;
			
			return -1? (pos == size_): pos;
		}

		// 寻找满足条件的值
		c_size find(const std::function<bool(const T&)>& check, c_size pos) const
		{
			assert_insize(pos, 0, size_ - 1);

			for (c_size i = pos; pos < size_; ++i)
				if (check(arr_[i]))
					return i;
			return -1;
		}

		T& operator[] (c_size index)
		{
			assert_insize(index, -size_, size_ - 1);

			index = (index + size_) % size_;
			return arr_[index];
		}

		const T& operator[] (c_size index) const
		{
			assert_insize(index, -size_, size_ - 1);

			index = (index + size_) % size_;
			return arr_[index];
		}

		// 切片，[l, r)
		Array slice(c_size l, c_size r) const
		{
			assert_insize(l, -size_, size_), assert_insize(r, -size_, size_);

			l = (l + size_) % size_;
			if (r != size_) r = (r + size_) % size_;
			
			c_size ret_size = r - l;
			if (ret_size < 0) ret_size = (ret_size + size_) % size_;
			Array<T> ret(ret_size);

			for (c_size i = 0; i < ret.size_; ++i)
				ret.arr_[i] = arr_[(l + i) % size_];

			return ret;
		}

		const char* __str__() const override
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << arr_[i];
			}
			stream << "]";
			return stream.str().c_str();
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
			assert_insize(size__, 0, MAX_ALLOC);
			release();
			if (size__) arr_ = new T[size__]{};
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
		T* arr_;

		c_size size_;
	};
}