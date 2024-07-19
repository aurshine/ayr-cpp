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
			static Array<c_size> INDEX_CACHE_IN_BLOCK = make_array<c_size>(CACHE_INDEX_BOUND + 1, [](c_size& x) {
				return highbit_index(x + 1);
				});

			if (index ^ CACHE_INDEX_BOUND)
				return highbit_index(index + 1);

			return INDEX_CACHE_IN_BLOCK[index];
		}
	};


	// 动态数组
	template<typename T>
	class DynArray: public IndexContainer<DynArray<T>, T>
	{
		using self = DynArray<T>;

		using super = IndexContainer<DynArray<T>, T>;

	public:
		DynArray() : dynarray_(sizeof(uint64_t) * 8), size_(0), occupies_size_(0) {}

		DynArray(const self& other) : dynarray_(other.dynarray_), size_(other.size_), occupies_size_(other.occupies_size_) {}

		DynArray(self&& other) : self() { swap(other); }

		self& operator=(const self& other)
		{
			dynarray_ = other.dynarray_;
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other)
				return *this;

			dynarray_ = std::move(other.dynarray_);
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		void swap(self& other)
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
			arr.fill(super::begin(), super::end());
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
		cmp_t __cmp__(const self& other) const
		{
			for (c_size i = 0; i < std::min(occupies_size_, other.occupies_size_); ++i)
			{
				c_size res = dynarray_[i].__cmp__(other.dynarray_[i]);
				if (res) return res;
			}

			return occupies_size_ - other.occupies_size_;
		}

		virtual self& __iter_container__() const { return const_cast<self&>(*this); }

	protected:
		// 对index范围不做检查
		T& __at__(c_size index)
		{
			c_size l = _BlockCache::get(index);
			++index;
			return dynarray_.arr_[l].arr_[index ^ exp2(l)];
		}

		// 对index范围不做检查
		const T& __at__(c_size index) const
		{
			c_size l = _BlockCache::get(index);
			++index;
			return dynarray_.arr_[l].arr_[index ^ exp2(l)];
		}

		// 唤醒一个新的块
		void __wakeup__()
		{
			occupies_size_++;

			auto&& block = dynarray_[occupies_size_ - 1];

			block.relloc(exp2(occupies_size_ - 1));
		}

	private:
		Array<Array<T>> dynarray_;

		c_size size_, occupies_size_;
	};
}