#pragma once
#include <functional>
#include <string>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>
#include <law/detail/Sequence.hpp>


namespace ayr
{
	constexpr static c_size MAX_ALLOC = LLONG_MAX;

	template<typename T>
	class Array : public Sequence<T>
	{
		using self = Array<T>;

		using super = Sequence<T>;
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

		T& __at__(c_size index) { return arr_[index]; }

		const T& __at__(c_size index) const { return arr_[index]; }

		// 切片[l, r)
		self slice(c_size l, c_size r) const
		{
			l = neg_index(l, size_), r = neg_index(r, size_);

			c_size ret_size = r - l;

			self ret = self::noconstruct(ret_size);
			for (c_size i = l; i < r; ++i)
				ret.arr_[i - l] = arr_[l];

			return ret;
		}

		// 输出的字符串形式
		CString __str__() const
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << arr_[i];
			}
			stream << "]";

			return CString(stream.str());
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

		// 先释放内存再分配
		virtual void relloc(c_size size)
		{
			release(size_);
			if (size) arr_ = ayr_alloc(T, size);
			size_ = size;
		}

		// 释放内存, size个元素调用析构函数
		virtual void release(c_size size)
		{
			if (arr_ == nullptr) return;

			for (c_size i = 0; i < size; ++ i)
				ayr_destroy(arr_ + i);
			ayr_delloc(arr_);

			arr_ = nullptr;
			size_ = 0;
		}

		T* arr_ = nullptr;

		c_size size_ = 0;
	};
}