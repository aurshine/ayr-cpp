#ifndef AYR_DETAIL_ARRAY_HPP
#define AYR_DETAIL_ARRAY_HPP

#include <algorithm>

#include <ayr/detail/printer.hpp>
#include <ayr/detail/ayr_memory.hpp>
#include <ayr/detail/Sequence.hpp>


namespace ayr
{
	template<typename T>
	class Array : public Sequence<Array<T>, T>
	{
		using self = Array<T>;

		using super = Sequence<self, T>;
	public:
		using Value_t = T;

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

		T& at(c_size index) { return data()[index]; }

		const T& at(c_size index) const { return data()[index]; }

		c_size size() const { return size_; }

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