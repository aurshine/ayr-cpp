#pragma once
#include <array>

#include <law/Array.hpp>

namespace ayr
{
	constexpr static size_t DYNARRAY_BLOCK_SIZE = 64;
	
	// 2^x
	constexpr size_t _exp2(int x) { return 1ull << x; }

	constexpr std::array<c_size, DYNARRAY_BLOCK_SIZE> make_exp()
	{
		std::array<c_size, DYNARRAY_BLOCK_SIZE> a;
		for (int i = 0; i < DYNARRAY_BLOCK_SIZE; ++i)
			a[i] = _exp2(i);
		return a;
	}

	// EXP[i] 第i位为1其余位为0
	constexpr static std::array<c_size, DYNARRAY_BLOCK_SIZE> EXP2 = make_exp();

	template<typename T>
	class DynArray: Object
	{
	public:
		DynArray() : dynarray_(EXP2.size()), size_(0), occupies_size_(0) {}
		
		// 容器存储的数据长度
		c_size size() const { return size_; }

		// 容器已经占用的块
		c_size occupy_size() const { return occupies_size_; }

		bool contains(const T& item) const { return find(item) != -1; }

		c_size find(const T& item) const 
		{
			for (c_size i = 0; i < size_; ++i)
				if (operator[](i) == item)
					return i;
			return -1;
		}

		T& operator[] (c_size index)
		{
			assert_insize(index, -size_, size_ - 1, "[]");

			index = (index + size_) % size_;

			return __at__(index);
		}

		const T& operator[] (c_size index) const
		{
			assert_insize(index, -size_, size_ - 1, "const []");

			index = (index + size_) % size_;

			return __at__(index);
		}
		
		void append(const T& item)
		{
			if (!((size_ + 1) & size_))
				__wakeup__();

			__at__(size_) = item;
			++size_;
		}

		void del()
		{

		}

		std::string __str__() const
		{
			std::stringstream stream;
			stream << "<DynArray> [";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << __at__(i);
			}
			stream << "]";

			return stream.str();
		}

		cmp_t __cmp__(const DynArray& other) const
		{
			for (c_size i = 0; i < std::min(occupies_size_, other.occupies_size_); ++i)
			{
				c_size res = dynarray_[i].__cmp__(other.dynarray_[i]);
				if (res) return res;
			}

			return occupies_size_ - other.occupies_size_;
		}

	protected:
		// 内部函数，对index范围不做检查
		T& __at__(c_size index)
		{
			++ index;
			c_size l = 0, r = occupies_size_ - 1;

			while (l < r)
			{
				c_size mid = l + r >> 1;
				if (index <= EXP2[mid]) r = mid;
				else l = mid + 1;
			}

			return dynarray_[l][EXP2[l] - index];
		}

		const T& __at__(c_size index) const
		{
			++index;
			c_size l = 0, r = occupies_size_ - 1;

			while (l < r)
			{
				c_size mid = l + r >> 1;
				if (index <= EXP2[mid]) r = mid;
				else l = mid + 1;
			}
			return dynarray_[l][EXP2[l] - index];
		}

		void __wakeup__()
		{
			occupies_size_++;
			assert_insize(occupies_size_, 1, EXP2.size() - 1, "__wakeup__");
			dynarray_[occupies_size_ - 1].relloc(EXP2[occupies_size_ - 1]);
		}

	private:
		Array<Array<T>> dynarray_;

		c_size size_, occupies_size_;
	};
}