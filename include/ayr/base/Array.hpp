#ifndef AYR_BASE_ARRAY_HPP
#define AYR_BASE_ARRAY_HPP

#include "Sequence.hpp"

namespace ayr
{
	template<typename T>
	class Array : public Sequence<Array<T>, T>
	{
		using self = Array<T>;

		using super = Sequence<self, T>;

		T* arr_;

		c_size size_;

		template<typename... Ts>
		friend def arr(Ts&&... values);
	public:
		using Value_t = T;

		Array(c_size size) : size_(size), arr_(ayr_alloc<T>(size))
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i);
		}

		Array(c_size size, const Value_t& value) : size_(size), arr_(ayr_alloc<T>(size))
		{
			for (c_size i = 0; i < size; ++i)
				ayr_construct(data() + i, value);
		}

		Array(std::initializer_list<T>&& init_list) : size_(init_list.size()), arr_(ayr_alloc<T>(init_list.size()))
		{
			c_size i = 0;
			for (const T& item : init_list)
				ayr_construct(data() + i++, item);
		}

		Array(const self& other) : size_(other.size_), arr_(ayr_alloc<T>(other.size_))
		{
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(data() + i, other.data()[i]);
		}

		Array(self&& other) noexcept : size_(other.size_), arr_(other.arr_) { other.size_ = 0; other.arr_ = nullptr; }

		~Array()
		{
			ayr_desloc(arr_, size_);
			size_ = 0;
		};

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other)
				return *this;

			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		T* data() { return arr_; }

		const T* data() const { return arr_; }

		T& at(c_size index) { return arr_[index]; }

		const T& at(c_size index) const { return arr_[index]; }

		c_size size() const { return size_; }

		// 重新分配内存，不保留原有数据
		void resize(c_size new_size)
		{
			ayr_desloc(arr_, size_);
			size_ = new_size;
			arr_ = ayr_alloc<T>(new_size);
			for (c_size i = 0; i < new_size; ++i)
				ayr_construct(data() + i);
		}

		// 分离数组，返回数组指针和大小，并将数组置空
		std::pair<T*, c_size> separate()
		{
			std::pair<T*, c_size> result = { data(), size_ };

			arr_ = nullptr;
			size_ = 0;
			return result;
		}

		std::strong_ordering operator<=>(const self& other) const { return super::operator<=>(other); }

		bool operator==(const self& other) const { return super::operator==(other); }
	};

	// 通过初始化列表构造数组
	template<typename T>
	def arr(std::initializer_list<T>&& init_list) { return Array<T>(std::move(init_list)); }

	// 通过变参模板构造数组
	template<typename... Ts>
	def arr(Ts&&... values)
	{
		using Type = std::common_type_t<Ts...>;
		Array<Type> res(0);
		res.size_ = sizeof...(values);
		res.arr_ = ayr_alloc<Type>(res.size_);

		c_size i = 0;
		(ayr_construct(res.arr_ + i++, std::forward<Ts>(values)), ...);
		return res;
	}
}
#endif // AYR_BASE_ARRAY_HPP