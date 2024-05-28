#pragma once
#include "Array.hpp"

namespace ayr
{
	template<typename T>
	class Dynarray : public Array<T>
	{
	public:
		Dynarray() : Array<T>(), capacity_(0) { }

		Dynarray(c_size size__) : Dynarray() { relloc(size__); }

		Dynarray(const T& fill_, c_size size__) : Array<T>(fill_, size__), capacity_(size__) {}

		Dynarray(T* raw_arr, c_size size__) : Array<T>(raw_arr, size__), capacity_(size__) {}

		Dynarray(const std::initializer_list<T>& init_list) : Array<T>(init_list), capacity_(init_list.size()) {}

		Dynarray(const Dynarray& other) : Dynarray() { *this = other; }

		Dynarray(Dynarray&& other) : Dynarray() { swap(other); }

		~Dynarray() { release(); }

		Dynarray& operator= (Dynarray&& other) { swap(other); return *this; }

		Dynarray& operator= (const Dynarray& other)
		{
			relloc(other.capacity_);
			this->size_ = other.size_;
			this->capacity_ = other.capacity_;
			this->fill(other.begin(), other.end());
			return *this;
		}

		void swap(Dynarray& other)
		{
			std::swap(this->arr_, other.arr_);
			std::swap(this->size_, other.size_);
			std::swap(this->capacity_, other.capacity_);
		}

		void swap(Dynarray&& other) { swap(other); }

		void append()
		{

		}

		void insert()
		{

		}

		void remove()
		{

		}
	protected:
		void relloc(c_size size__) override
		{
			assert_insize(size__, 0, MAX_ALLOC);
			release();
			if (size__) this->arr_ = new T[size__]{};
			this->capacity_ = size__;
			this->size_ = 0;
		}

		void release() override
		{
			delete[] this->arr_;
			this->arr_ = nullptr;
			this->capacity_ = this->size_ = 0;
		}

		c_size capacity_;
	};
}