#ifndef AYR_LAW_DETAIL_ARRAY_HPP
#define AYR_LAW_DETAIL_ARRAY_HPP

#include <law/detail/printer.hpp>
#include <law/detail/ayr_memory.hpp>
#include <law/detail/Sequence.hpp>


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

		T& __at__(c_size index) override { return data()[index]; }

		const T& __at__(c_size index) const override { return data()[index]; }

		virtual CString __str__() const override
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size(); ++i)
			{
				if (i != 0) stream << ", ";
				stream << __at__(i);
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


	public:
		Array_(c_size size) : size_(size), arr_(std::make_unique<T[]>(size)) {}

		template<typename ...Args>
		Array_(c_size size, const Args&... args) : Array_(size)
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i, args...);
		}

		Array_(std::initializer_list<T>&& init_list) : Array_(init_list.size())
		{
			c_size i = 0;
			for (auto&& item : init_list)
				ayr_construct(data() + i++, std::move(item));
		}

		Array_(const self& other) : Array_(other.size())
		{
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(data() + i, other.data()[i]);
		}

		Array_(self&& other) noexcept : size_(other.size_), arr_(std::move(other.arr_)) {}

		~Array_() = default;

		T* data() override { return arr_.get(); }

		const T* data() const override { return arr_.get(); }

		c_size size() const override { return size_; }
	private:
		std::unique_ptr<T[]> arr_;

		c_size size_ = 0;
	};


	template<typename T, size_t N = 0>
	using Array = Array_<T, N>;
}
#endif