#pragma once
#include <algorithm>
#include <string>
#include "printer.hpp"
#include "container.hpp"

namespace ayr
{
	template<typename T>
	class Array : public Container<T>, public Comparator<Array<T>>
	{
	public:
		Array(): arr_(nullptr), size_(0) {}
		
		Array(c_size size__) : arr_(new T[size__]), size_(size__) {}

		Array(c_size size__, const T& fill_): Array(size__) { fill(fill_); }

		Array(T* raw_arr, c_size size__): arr_(raw_arr), size_(size__) {}

		Array(const std::initializer_list<T>& init_list) : Array(init_list.size()) { fill(init_list); }

		Array(const Array& other): Array(other.size_) { fill(other); }

		Array(Array&& other) : Array(other.arr_, other.size_) { other.arr_ = nullptr, other.size_ = 0; }

		~Array() { release(); }

		c_size size() const { return size_; }

		void fill(const T& fill_val)
		{
			for (c_size i = 0; i < size_; ++i)
				arr_[i] = fill_val;
		}

		void fill(const Array& other)
		{
			c_size min_size = std::min(size_, other.size_);
			for (c_size i = 0; i < min_size; ++i)
				arr_[i] = other.arr_[i];
		}

		void fill(const Array& other, const T& default_)
		{
			for (c_size i = 0; i < size_; ++i)
				if (i < other.size_)
					arr_[i] = other.arr_[i];
				else
					arr_[i] = default_;
		}

		template<typename Iterable>
		void fill(Iterable&& iter)
		{
			c_size idx = 0;
			for (auto it : iter)
			{
				if (idx == size_) break;
				arr_[idx++] = std::forward<T>(it);
			}
		}

		template<typename Iterable>
		void fill(Iterable&& iter, const T& default_)
		{
			c_size idx = 0;
			for (auto it : iter)
			{
				if (idx == size_) break;
				arr_[idx++] = std::forward<T>(it);
			}

			while (idx < size_) arr_[idx++] = default_;
		}

		bool contains(const T& item) const override
		{
			for (c_size i = 0; i < size_; ++i)
				if (arr_[i] == item)
					return true;
			return false;
		}

		T& operator[] (c_size index)
		{
			assert_insize(index);
			
			index = (index + size_) % size_;
			return arr_[index]; 
		}

		const T& operator[] (c_size index) const
		{
			assert_insize(index);

			index = (index + size_) % size_;
			return arr_[index];
		}

		// ÇÐÆ¬£¬[l, r)
		Array slice(c_size l, c_size r)
		{
			assert_insize(l), assert_insize(r);

			l = (l + size_) % size_, r = (r + size_) % size_;

			Array<T> ret((r - l + size_) % size_);
			
			for (c_size i = 0; i < ret.size_; ++i)
				ret.arr_[i] = arr_[(l + i) % size_];

			return ret;
		}

		std::string __str__() const override
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << arr_[i];
			}
			stream << "]";
			return stream.str();
		}

		c_size __cmp__(const Array<T>& other) const override
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

	private:
		void release() { delete[] arr_; size_ = 0; }
		
		void assert_insize(c_size x) const 
		{
			error_assert(x >= -size_ && x < size_, std::format("{} out of range [{}, {})", x, -size_, size_)); 
		};

		T* arr_;

		c_size size_;
	};
}