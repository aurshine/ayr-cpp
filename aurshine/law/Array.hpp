#pragma once
#include <algorithm>
#include <functional>
#include <iterator>
#include <string>
#include <array>


#include <law/printer.hpp>


namespace ayr
{
	constexpr static c_size MAX_ALLOC = LLONG_MAX;

#ifdef AYR_DEBUG
	// 检查 x 在[l, r] 区间
#define assert_insize(x, lbound, rbound) error_assert((x) >= (lbound) && (x) <= (rbound), std::format("{} out of range [{}, {}]", (x), (lbound), (rbound)))
#else
#define assert_insize(x, lbound, rbound)
#endif

	template<typename T>
	class Array : public Object
	{
	public:
		using iterator = T*;

		using const_iterator = const T*;

		Array() : arr_(nullptr), size_(0) {}

		Array(c_size size__) : Array() { relloc(size__); }

		Array(c_size size__, const T& fill_) : Array(size__) { fill(fill_); }

		Array(c_size size__, T* raw_arr) : arr_(raw_arr), size_(size__) {}

		Array(const std::initializer_list<T>& init_list) : Array(init_list.size()) { fill(init_list.begin(), init_list.end()); }

		Array(const Array& other) : Array(other.size_) { fill(other); }

		Array(Array&& other) : Array() { swap(other); }

		~Array() { release(); }

		Array& operator= (const Array& other)
		{
			if (this == &other) return *this;

			release();
			relloc(other.size_);
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

		void swap(Array&& other) { swap(other); }

		// fill_val 填充值
		void fill(const T& fill_val, c_size pos = 0)
		{
			assert_insize(pos, 0, size_);

			while (pos < size_) arr_[pos++] = fill_val;
		}

		// 以迭代器填充元素
		template<typename It>
		void fill(It begin, It end, c_size pos = 0)
		{
			while (pos < size_ && begin != end)
			{
				arr_[pos++] = *begin;
				++begin;
			}
		}

		// 以迭代器填充元素, 未填满使用默认值
		template<typename It>
		void fill(It begin, It end, const T& default_, c_size pos = 0)
		{
			while (pos < size_ && begin != end)
			{
				arr_[pos++] = *begin;
				++begin;
			}

			fill(default_, pos);
		}

		// 以Array对象填充元素
		void fill(const Array& other, c_size pos = 0)
		{
			fill(other.begin(), other.end(), pos);
		}

		// 以Array对象填充元素, 未填满使用默认值
		void fill(const Array& other, const T& default_, c_size pos = 0)
		{
			fill(other.begin(), other.end(), default_, pos);
		}

		// 寻找元素返回下标
		c_size find(const T& find_item, c_size pos = 0) const
		{
			assert_insize(pos, 0, size_ - 1);

			while (pos < size_ && arr_[pos] != find_item) ++pos;

			return -1 ? (pos == size_) : pos;
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

		// 切片[l, r]
		// 特殊的 arr = [1, 2, 3, 4] => arr.slice(2, 1) = [3, 4, 1, 2]
		Array slice(c_size l, c_size r) const
		{
			assert_insize(l, -size_, size_ - 1);
			assert_insize(r, -size_, size_ - 1);

			l = (l + size_) % size_;
			if (r != size_) r = (r + size_) % size_;

			c_size ret_size = r - l + 1;
			if (ret_size < 0)
				ret_size = (ret_size + size_) % size_;
			else if (ret_size == 0)
				ret_size = size_;

			Array<T> ret(ret_size);

			for (c_size i = 0; i < ret.size_; ++i)
				ret.arr_[i] = arr_[(l + i) % size_];

			return ret;
		}

		// 输出的字符串形式
		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<Array> [";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << arr_[i];
			}
			stream << "]";

			memcpy__str_buffer__(stream.str());

			return __str_buffer__;
		}

		// 比较逻辑
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

		// 容器大小
		c_size size() const { return size_; }

		// 容器底层指针
		T* ptr() { return arr_; }

		// 容器底层指针
		const T* ptr() const { return arr_; }

		// 元素是否存在
		bool contains(const T& item) const { return find(item) != -1; }

		// 迭代器
		iterator begin() { return arr_; }

		iterator end() { return arr_ + size_; }

		const_iterator begin() const { return arr_; }

		const_iterator end() const { return arr_ + size_; }

		std::reverse_iterator<iterator> rbegin() { return arr_ + size_ - 1; }

		std::reverse_iterator<iterator> rend() { return arr_ - 1; }

		std::reverse_iterator<const_iterator> rbegin() const { return arr_ + size_ - 1; }

		std::reverse_iterator<const_iterator> rend() const { return arr_ - 1; }

		// 先释放再分配
		virtual void relloc(c_size size__)
		{
			assert_insize(size__, 0, MAX_ALLOC);
			release();
			if (size__) arr_ = new T[size__]{};
			size_ = size__;
		}

		// 释放内存
		virtual void release()
		{
			delete[] arr_;
			arr_ = nullptr;
			size_ = 0;
		}

		T* arr_;

		c_size size_;
	};


	class Range : public Object
	{
	public:
		Range(c_size start, c_size end, c_size step = 1) : _start(start), _end(end), _step(step) {}

		Range(c_size end) : Range(0, end, 1) {}

		class RangeIterator
		{
		public:
			RangeIterator(c_size current, c_size step) : current_(current), step_(step) {}

			c_size operator*() const { return current_; }

			RangeIterator& operator++()
			{
				current_ += step_;

				return *this;
			}

			bool operator!=(const RangeIterator& other) const { return current_ != other.current_; }

			cmp_t __cmp__(const RangeIterator& other) const { return current_ - other.current_; }

		private:
			c_size current_, step_;
		};

		RangeIterator begin() const { return RangeIterator(_start, _step); }

		RangeIterator end() const { return RangeIterator(_start + (_end - _start + _step - 1) / _step * _step, _step); }
	private:
		c_size _start, _end, _step;
	};


	template<typename T, size_t N, typename F>
	inline constexpr std::array<T, N> make_stl_array(const F& constexpr_func)
	{
		std::array<T, N> a;
		for (int i = 0; i < N; ++i)
			a[i] = constexpr_func(i);
		return a;
	}


	template<typename T, typename F>
	inline Array<T> make_array(c_size size, const F& func)
	{
		Array<T> arr(size);
		for (c_size i = 0; i < size; ++i)
			arr[i] = func(i);

		return arr;
	}
}