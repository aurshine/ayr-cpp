#pragma once
#include <utility>

#include <law/detail/bunit.hpp>
#include <law/Array.hpp>


namespace ayr
{
	// 动态数组块的数量
	constexpr static size_t DYNARRAY_BLOCK_SIZE = 64;


	struct _BlockCache : public Object
	{
		constexpr static auto CACHE_INDEX_BOUND = 0xffff;

		static c_size get(c_size index)
		{
			static auto INDEX_CACHE_IN_BLOCK = make_array<c_size>(CACHE_INDEX_BOUND + 1, [](c_size& x) {
				return highbit_index(x + 1);
				});

			if (index ^ CACHE_INDEX_BOUND)
				return highbit_index(index + 1);

			return INDEX_CACHE_IN_BLOCK[index];
		}
	};


	// 动态数组迭代器
	template <typename T>
	class DynArrayIterator;


	// 动态数组
	template<typename T>
	class DynArray : Object
	{
	public:
		friend class DynArrayIterator<T>;

		using iterator = DynArrayIterator<T>;

		using const_iterator = const DynArrayIterator<T>;

		DynArray() : dynarray_(EXP2.size()), size_(0), occupies_size_(0) {}

		DynArray(const DynArray& other) : dynarray_(other.dynarray_), size_(other.size_), occupies_size_(other.occupies_size_) {}

		DynArray(DynArray&& other) : DynArray() { swap(other); }

		DynArray& operator=(const DynArray& other)
		{
			dynarray_ = other.dynarray_;
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		DynArray& operator=(DynArray&& other)
		{
			if (this == &other)
				return *this;

			dynarray_ = std::move(other.dynarray_);
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		void swap(DynArray& other)
		{
			dynarray_.swap(other.dynarray_);
			std::swap(size_, other.size_);
			std::swap(occupies_size_, other.occupies_size_);
		}

		// 容器存储的数据长度
		c_size size() const { return size_; }

		// 容器已经占用的块
		c_size occupy_size() const { return occupies_size_; }

		// 容器是否包含item
		bool contains(const T& item) const { return find(item) != -1; }

		// 查找item的下标，不存在返回-1
		c_size find(const T& item) const
		{
			for (c_size i = 0; i < size_; ++i)
				if (__at__(i) == item)
					return i;
			return -1;
		}

		// 获取指定下标的元素，可以传入负数
		T& operator[] (c_size index)
		{
			assert_insize(index, -size_, size_ - 1);

			index = (index + size_) % size_;

			return __at__(index);
		}

		// 获取指定下标的元素，可以传入负数
		const T& operator[] (c_size index) const
		{
			assert_insize(index, -size_, size_ - 1);

			index = (index + size_) % size_;

			return __at__(index);
		}

		// 追加元素
		T& append(const T& item)
		{
			if (!((size_ + 1) & size_))
				__wakeup__();

			return __at__(size_++) = item;
		}

		T& append(T&& item)
		{
			if (!((size_ + 1) & size_))
				__wakeup__();

			return __at__(size_++) = std::move(item);
		}

		// 移除最后一个元素并返回
		T pop()
		{
			assert_insize(size_, 1, MAX_ALLOC);
			return __at__(--size_);
		}

		// 移除指定位置的元素并返回
		T pop(c_size index)
		{
			assert_insize(index, -size_, size_ - 1);

			index = (index + size_) % size_;
			T ret = __at__(index);

			for (c_size i = index; i < size_ - 1; ++i)
				__at__(i) = __at__(i + 1);
			--size_;

			return ret;
		}

		// 转换为Array
		Array<T> to_array() const
		{
			Array<T> arr(size_);
			arr.fill(begin(), end());
			return arr;
		}

		// 容器的字符串形式
		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<DynArray> [";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << __at__(i);
			}
			stream << "]";

			memcpy__str_buffer__(stream.str());

			return __str_buffer__;
		}

		// 容器的比较方式
		cmp_t __cmp__(const DynArray& other) const
		{
			for (c_size i = 0; i < std::min(occupies_size_, other.occupies_size_); ++i)
			{
				c_size res = dynarray_[i].__cmp__(other.dynarray_[i]);
				if (res) return res;
			}

			return occupies_size_ - other.occupies_size_;
		}

		// 迭代器
		iterator begin() { return iterator(this); }

		iterator end() { return iterator(this, size_); }

		std::reverse_iterator<const_iterator> rbegin() const { return iterator(this, size_ - 1); }

		std::reverse_iterator<const_iterator> rend() const { return iterator(this, -1); }

	protected:
		// 对index范围不做检查
		T& __at__(c_size index)
		{
			c_size l = _BlockCache::get(index);
			++index;
			return dynarray_.arr_[l].arr_[index ^ EXP2[l]];
		}

		// 对index范围不做检查
		const T& __at__(c_size index) const
		{
			c_size l = _BlockCache::get(index);
			++index;
			return dynarray_.arr_[l].arr_[index ^ EXP2[l]];
		}

		// 唤醒一个新的块
		void __wakeup__()
		{
			occupies_size_++;

			auto&& block = dynarray_[occupies_size_ - 1];

			block.relloc(EXP2[occupies_size_ - 1]);
		}

	private:
		Array<Array<T>> dynarray_;

		c_size size_, occupies_size_;
	};


	template <typename T>
	class DynArrayIterator : public Object
	{
	public:
		DynArrayIterator(const DynArray<T>* dynarray, c_size cur = 0) : dynarray_(const_cast<DynArray<T>*>(dynarray)), cur_(cur) {}

		DynArrayIterator(const DynArrayIterator<T>& other) : dynarray_(other.dynarray_), cur_(other.cur_) {}

		DynArrayIterator& operator==(const DynArrayIterator<T>& other) const
		{
			this->dynarray_ = other.dynarray_;
			this->cur_ = other.cur_;
			return *this;
		}

		DynArrayIterator operator++(int)
		{
			auto temp = *this;
			++cur_;
			return temp;
		}

		DynArrayIterator& operator++()
		{
			++cur_;
			return *this;
		}

		DynArrayIterator operator--(int)
		{
			auto temp = *this;
			--cur_;
			return temp;
		}

		DynArrayIterator& operator--()
		{
			++cur_;
			return *this;
		}

		DynArrayIterator operator+(c_size n) const { return DynArrayIterator(dynarray_, cur_ + n); }

		DynArrayIterator& operator+=(c_size n)
		{
			cur_ += n;
			return *this;
		}

		DynArrayIterator operator-(c_size n) const { return DynArrayIterator(dynarray_, cur_ - n); }

		DynArrayIterator& operator-=(c_size n)
		{
			cur_ -= n;
			return *this;
		}

		T& operator*() const { return dynarray_->operator[](cur_); }

		T* operator->() const { return &dynarray_->operator[](cur_); }

		c_size operator-(const DynArrayIterator<T>& other) const { return cur_ - other.cur_; }

		cmp_t __cmp__(const DynArrayIterator<T>& other) const
		{
			if (dynarray_ == other.dynarray_)
				return cur_ - other.cur_;
			return dynarray_ - other.dynarray_;
		}

		bool operator!=(const DynArrayIterator<T>& other) const { return __cmp__(other) != 0; }

	private:
		DynArray<T>* dynarray_;

		c_size cur_;
	};
}