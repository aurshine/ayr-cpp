#ifndef AYR_LAW_DETAIL_ARRAY_HPP
#define AYR_LAW_DETAIL_ARRAY_HPP

#include <sstream>

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>
#include <law/detail/Sequence.hpp>


namespace ayr
{
	template<typename T>
	class ArrayInterface: public Object
	{
		virtual constexpr T* data() = 0;
		
		virtual constexpr const T* data() const = 0;

		virtual constexpr c_size size() const = 0;

		constexpr T& operator[] (c_size index) { return __at__(neg_index(index, size())); }

		constexpr const T& operator[] (c_size index) const { return __at__(neg_index(index, size())); }

		constexpr void fill(const T& fill_val, c_size pos = 0)
		{
			T* arr = data();
			for (c_size i = pos; i < size(); ++i)
				arr[i] = fill_val;
		}

		constexpr T* begin() { return data(); }
		
		constexpr T* end() { return data() + size(); }

		constexpr const T* begin() const { return data(); }

		constexpr const T* end() const { return data() + size(); }

		constexpr T& __at__(c_size index) { return data()[index]; }

		constexpr const T& __at__(c_size index) const { return data()[index]; }
	};

	/*
	* 静态数组
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


	template<typename T>
	class ArrayImpl<T, 0>: public ArrayInterface<T>
	{
		using self = ArrayImpl<T, 0>;

		using super = ArrayInterface<T>;

		T* arr_ = nullptr;

		c_size size_ = 0;
	public:
		ArrayImpl(c_size size)
		{

		}


		T* data() override { return arr_; }

		const T* data() const override { return arr_; }

		c_size size() const override { return size_; }
		

	};

	template<typename T, size_t N = 0>
	using Array = ArrayImpl<T, N>;
}
#endif