#ifndef AYR_LAW_DETAIL_ARRAY_HPP
#define AYR_LAW_DETAIL_ARRAY_HPP

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>
#include <law/detail/Sequence.hpp>


namespace ayr
{
	template<typename T>
	class ArrayInterface: public Object
	{
	public:
		virtual T* data() = 0;
		
		virtual const T* data() const = 0;

		virtual c_size size() const = 0;

		T& operator[] (c_size index) { return __at__(neg_index(index, size())); }

		const T& operator[] (c_size index) const { return __at__(neg_index(index, size())); }

		void fill(const T& fill_val, c_size pos = 0)
		{
			T* arr = data();
			for (c_size i = pos; i < size(); ++i)
				arr[i] = fill_val;
		}

		T* begin() { return data(); }
		
		T* end() { return data() + size(); }

		const T* begin() const { return data(); }

		const T* end() const { return data() + size(); }

		T& __at__(c_size index) { return data()[index]; }

		const T& __at__(c_size index) const { return data()[index]; }
	};

	/*
	* 栈上分配内存
	* 
	* 元素类型T, 大小N
	* 
	* 所有操作均可编译期完成
	*/
	template<typename T, size_t N>
	class ArrayImpl : public ArrayInterface<T>
	{
		using self = ArrayImpl<T, N>;

		using super = ArrayInterface<T>;
		
		T arr_[N];
	public:
		ArrayImpl() = default;

		ArrayImpl(const T& fill_val) { super::fill(fill_val, 0); }

		ArrayImpl(std::initializer_list<T>&& init_list): arr_(init_list) {}

		constexpr T* data() override { return arr_; }

		constexpr const T* data() const override { return arr_; }

		constexpr c_size size() const override { return N; }
	};

	/*
	* 堆上分配内存
	* 
	* 元素类型T, 大小N
	* 
	* 当只传入数组长度时，只分配内存，不调用构造函数
	* 
	* release()只能有效调用一次，用于调用一个区间的析构函数，并释放内存
	*/
	template<typename T>
	class ArrayImpl<T, 0>: public ArrayInterface<T>
	{
		using self = ArrayImpl<T, 0>;

		using super = ArrayInterface<T>;

		T* arr_ = nullptr;

		c_size size_ = 0;

		ArrayImpl(c_size size) : arr_(ayr_alloc(T, size)), size_(size) {}
	public:
		template<typename ...Args>
		ArrayImpl(c_size size, const Args&... args): ArrayImpl(size)
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(arr_ + i, args...);
		}

		ArrayImpl(std::initializer_list<T>&& init_list) : ArrayImpl(init_list.size())
		{
			c_size i = 0;
			for (auto&& item: init_list)
				ayr_construct(arr_ + i ++, std::move(item));
		}

		~ArrayImpl() 
		{ 
			release();
			ayr_delloc(arr_); 
		}

		T* data() override { return arr_; }

		const T* data() const override { return arr_; }

		c_size size() const override { return size_; }

		void release(c_size l = 0, c_size r = size_)
		{
			static bool has_called = false;
			if (has_called) return;
			
			while (l < r) ayr_destroy(arr_ + (l ++));
		}
	};


	template<typename T, size_t N = 0>
	using Array = ArrayImpl<T, N>;
}
#endif