#ifndef AYR_DETAIL_ARRAY_HPP
#define AYR_DETAIL_ARRAY_HPP

#include <algorithm>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/ayr_memory.hpp>
#include <ayr/detail/Sequence.hpp>


namespace ayr
{
	/*
	* 栈上分配内存
	*
	* 元素类型T, 大小N
	*
	* 所有操作均可编译期完成
	*/
	template<typename T, size_t N>
	class SArray : public Sequence<T>
	{
		using self = SArray<T, N>;

		using super = Sequence<T>;

		T arr_[N];
	public:
		constexpr SArray() {};

		constexpr SArray(const T& fill_val)
		{
			for (c_size i = 0; i < N; ++i)
				arr_[i] = fill_val;
		}

		constexpr SArray(std::initializer_list<T>&& init_list)
		{
			for (c_size i = 0; i < N; ++i)
				arr_[i] = std::move(*(init_list.begin() + i));
		}

		constexpr T* data() { return arr_; }

		constexpr const T* data() const { return arr_; }

		constexpr c_size size() const override { return N; }

		T& at(c_size index) override { return data()[index]; }

		const T& at(c_size index) const override { return data()[index]; }

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
	class Array : public Sequence<T>
	{
		using self = Array<T>;

		using super = Sequence<T>;
	public:
		template<typename ...Args>
		Array(c_size size, const Args&... args) : size_(size), arr_(std::make_unique<T[]>(size))
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i, args...);
		}

		Array(std::initializer_list<T>&& init_list) : size_(init_list.size()), arr_(std::make_unique<T[]>(init_list.size()))
		{
			c_size i = 0;
			for (auto&& item : init_list)
				ayr_construct(data() + i++, std::move(item));
		}

		Array(const self& other) : size_(other.size_), arr_(std::make_unique<T[]>(other.size_))
		{
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(data() + i, other.data()[i]);
		}

		Array(self&& other) noexcept : size_(other.size_), arr_(std::move(other.arr_)) { other.size_ = 0; }

		~Array() { size_ = 0; };

		T* data() { return arr_.get(); }

		const T* data() const { return arr_.get(); }

		T& at(c_size index) override { return data()[index]; }

		const T& at(c_size index) const override { return data()[index]; }

		c_size size() const override { return size_; }

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

			arr_.release();
			size_ = 0;
			return result;
		}
	private:
		std::unique_ptr<T[]> arr_;

		c_size size_ = 0;
	};
}
#endif