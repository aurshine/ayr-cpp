#ifndef AYR_DETAIL_ARRAY_HPP
#define AYR_DETAIL_ARRAY_HPP

#include <algorithm>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/ayr_memory.hpp>
#include <ayr/detail/Sequence.hpp>


namespace ayr
{
	template<typename T>
	class ArrayImpl : public Sequence<T>
	{
		using self = ArrayImpl<T>;

		using super = Sequence<T>;
	public:
		virtual T* data() = 0;

		virtual const T* data() const = 0;

		virtual c_size size() const = 0;

		void fill(const T& fill_val, c_size pos = 0)
		{
			T* arr = data();
			for (c_size i = pos; i < size(); ++i)
				arr[i] = fill_val;
		}

		T& at(c_size index) override { return data()[index]; }

		const T& at(c_size index) const override { return data()[index]; }

		virtual CString __str__() const
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
	};

	/*
	* 栈上分配内存
	*
	* 元素类型T, 大小N
	*
	* 所有操作均可编译期完成
	*/
	template<typename T, size_t N>
	class Array_ : public ArrayImpl<T>
	{
		using self = Array_<T, N>;

		using super = ArrayImpl<T>;

		T arr_[N];
	public:
		Array_() = default;

		Array_(const T& fill_val) { super::fill(fill_val, 0); }

		Array_(std::initializer_list<T>&& init_list) : arr_(init_list) {}

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
	class Array_<T, 0> : public ArrayImpl<T>
	{
		using self = Array_<T, 0>;

		using super = ArrayImpl<T>;

		// 构造函数, 每个元素不会调用构造函数
		Array_(c_size size, std::false_type) : size_(size), arr_(ayr_alloc<T>(size)) {}
	public:
		template<typename ...Args>
		Array_(c_size size, const Args&... args) : Array_(size, std::false_type{})
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i, args...);
		}

		Array_(std::initializer_list<T>&& init_list) : Array_(init_list.size(), std::false_type{})
		{
			c_size i = 0;
			for (auto&& item : init_list)
				ayr_construct(data() + i++, std::move(item));
		}

		Array_(T* arr, c_size size) : size_(size), arr_(arr) {}

		Array_(const self& other) : Array_(other.size(), std::false_type{})
		{
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(data() + i, other.data()[i]);
		}

		Array_(self&& other) noexcept : size_(other.size_), arr_(std::move(other.arr_)) { other.arr_ = nullptr; other.size_ = 0; }

		~Array_() { ayr_delloc(arr_); size_ = 0; };

		T* data() override { return arr_; }

		const T* data() const override { return arr_; }

		c_size size() const override { return size_; }

		template<typename ...Args>
		void resize(c_size new_size, const Args&... args)
		{
			ayr_destroy(this);
			ayr_construct(this, new_size, args...);
		}

		// 分离数组，返回数组指针和大小，并将数组置空
		std::pair<T* c_size> separate()
		{
			std::pair<T* c_size> result = { arr_, size_ };
			arr_ = nullptr;
			size_ = 0;
			return result;
		}
	private:
		T* arr_;

		c_size size_ = 0;
	};


	template<typename T, size_t N = 0>
	using Array = Array_<T, N>;
}
#endif