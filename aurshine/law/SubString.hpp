#pragma once
#include <law/AString.hpp>

namespace ayr
{
	template<Char T>
	class SubString : public IndexContainer<SubString<T>, T>
	{
		using self = SubString<T>;
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

		CString __str__() const { return CString(reinterpret_cast<char*>(substr_), size_ * sizeof(T)); }

		cmp_t __cmp__(const SubString& other) const
		{
			for (size_t i = 0; substr_[i] || other.substr_[i]; i++)
				if (substr_[i] != other.substr_[i])
					return substr_[i] - other.substr_[i];
			return 0;
		}

		size_t __hash__() const { return bytes_hash(substr_, size_); }

		self& __iter_container__() const override { return const_cast<self&>(*this); }
	private:
		T* substr_;

		size_t size_;
	};
}