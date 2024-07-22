#pragma once
#include <functional>
#include <string>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>
#include <law/detail/IndexIterator.hpp>


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
	class Array : public IndexContainer<Array<T>, T>
	{
		using self = Array<T>;

		using super = IndexContainer<Array<T>, T>;
	public:
		Array(c_size size)
		{ 
			relloc(size);
			for (c_size i = 0; i < size_; ++ i)
				ayr_construct(T, arr_ + i);
		}

		Array(c_size size, const T& fill_)
		{ 
			relloc(size);
			for (c_size i = 0; i < size_; ++ i)
				ayr_construct(T, arr_ + i, fill_);
		}

		Array(std::initializer_list<T>&& init_list)
		{ 
			relloc(init_list.size());
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(T, arr_ + i, std::move(*(init_list.begin() + i)));
		}

		Array(const Array& other) 
		{ 
			relloc(other.size());
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(T, arr_ + i, other.arr_[i]);
		}

		Array(Array&& other) noexcept
		{ 
			arr_ = other.arr_;
			size_ = other.size_;
			other.arr_ = nullptr;
			other.size_ = 0;
		}

		~Array() { release(size_); }

		self& operator= (const self& other)
		{
			if (this == &other) return *this;

			relloc(other.size_);
			for (c_size i = 0; i < size_; ++i)
				arr_[i] = other.arr_[i];
			return *this;
		}

		self& operator= (self&& other)
		{
			if (this == &other) return *this;

			relloc(other.size_);
			for (c_size i = 0; i < size_; ++i)
				arr_[i] = std::move(other.arr_[i]);

			other.arr_ = nullptr;
			other.size_ = 0;
			return *this;
		}

		void swap(self& other)
		{
			std::swap(arr_, other.arr_);
			std::swap(size_, other.size_);
		}

		void swap(self&& other) { swap(other); }

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

		// 以Array对象填充元素
		void fill(const self& other, c_size pos = 0)
		{
			fill(other.begin(), other.end(), pos);
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
		self slice(c_size l, c_size r) const
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

			self ret(ret_size);

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
		c_size __cmp__(const self& other) const
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
		
		// 不用构造函数构造对象
		static self noconstruct(c_size size) 
		{
			self ret{};
			ret.relloc(size);
			return ret;
		}

		// 容器大小
		c_size size() const { return size_; }

		// 容器底层指针
		T* ptr() { return arr_; }

		// 容器底层指针
		const T* ptr() const { return arr_; }

		// 元素是否存在
		bool contains(const T& item) const { return find(item) != -1; }

		// 返回迭代容器
		self& __iter_container__() const override { return const_cast<self&>(*this); }

		// 先释放内存再分配
		virtual void relloc(c_size size)
		{
			assert_insize(size, 0, MAX_ALLOC);
			release(size_);
			if (size) arr_ = ayr_alloc(T, size);
			size_ = size;
		}

		// 释放内存, size个元素调用析构函数
		virtual void release(c_size size)
		{
			if (arr_ == nullptr) return;

			for (size_t i = 0; i < size; ++ i)
				ayr_destroy(arr_ + i);
			ayr_delloc(arr_);

			arr_ = nullptr;
			size_ = 0;
		}

		T* arr_ = nullptr;

		c_size size_ = 0;
	};
}