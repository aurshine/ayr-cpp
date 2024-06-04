#pragma once
#include <law/AString.hpp>

namespace ayr
{
	template<Char T>
	class SubString : Object
	{
	public:
		SubString(const T* str, size_t size) : substr_(str), size_(size) {}
		
		SubString(const SubString& other) : substr_(other.substr_), size_(other.size_) {}

		SubString& operator=(const SubString& other)
		{
			substr_ = other.substr_;
			size_ = other.size_;
			return *this;
		}

		void swap(SubString& other)
		{
			std::swap(substr_, other.substr_);
			std::swap(size_, other.size_);
		}

		T& operator[] (c_size index) { return substr_[index]; }

		const T& operator[] (c_size index) const { return substr_[index]; }

		c_size size() const { return substr_.size_; }

		T* ptr() { return substr_; }

		const T* ptr() const { return substr_; }

		SubString slice(c_size l, c_size r)
		{
			assert_insize(l, 0, size_ - 1);
			assert_insize(r, 0, size_ - 1);

			return SubString(substr_ + l, r - l + 1);
		}

		const char* __str__() const
		{
			memcpy__str_buffer__(substr_, size_);
			return __str_buffer__;
		}

		cmp_t __cmp__(const SubString& other) const
		{
			for (size_t i = 0; substr_[i] || other.substr_[i; i++)
				if (substr_[i] != other.substr_[i])
					return substr_[i] - other.substr_[i];
			return 0;
		}

		size_t __hash__() const
		{
			static size_t P = 1331;
			size_t hash_value = 0;
			for (size_t i = 0; substr_[i] != '\0'; i++)
				hash_value = hash_value * P + substr_[i];

			return hash_value;
		}

		T* begin() { return substr_; }

		T* end() { return substr_ + size_; }

		const T* begin() const { return substr_; }

		const T* end() const { return substr_ + size_; }

		std::reverse_iterator<T*> rbegin() { return substr_; }

		std::reverse_iterator<T*> rend() { return substr_ + size_; }

		std::reverse_iterator<const T*> rbegin() const { return substr_; }
	
		std::reverse_iterator<const T*> rend() const { return substr_ + size_; }
	private:
		T* substr_;

		size_t size_;
	};
}