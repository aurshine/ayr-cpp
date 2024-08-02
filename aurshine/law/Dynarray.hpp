#pragma once
#include <utility>

#include <law/detail/bunit.hpp>
#include <law/Array.hpp>


namespace ayr
{
	// 动态数组块的数量
	constexpr static size_t DYNARRAY_BLOCK_SIZE = sizeof(size_t) * 8;


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
	class DynArray : public Sequence<T>
	{
		using self = DynArray<T>;

		using super = Sequence<T>;

	public:
		DynArray() : dynarray_(DYNARRAY_BLOCK_SIZE, Array<T>(0)), size_(0), occupies_size_(0) {}

		DynArray(const self& other) : DynArray()
		{
			for (auto& item : other)
				append(item);
		}


		DynArray(self&& other) : dynarray_(std::move(other.dynarray_)), size_(other.size_), occupies_size_(other.occupies_size_)
		{
			other.size_ = 0;
			other.occupies_size_ = 0;
		}

		~DynArray() { release(); }

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			release();
			dynarray_ = other.dynarray_;
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other)
				return *this;

			release();
			dynarray_ = std::move(other.dynarray_);
			size_ = other.size_;
			occupies_size_ = other.occupies_size_;

			other.size_ = 0;
			other.occupies_size_ = 0;

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

		// 追加元素
		T& append(const T& item)
		{
			if (all_one(size_))
				__wakeup__();

			T& v = __at__(size_++);
			ayr_construct(T, &v, item);
			return v;
		}

		T& append(T&& item)
		{
			if (all_one(size_))
				__wakeup__();

			T& v = __at__(size_++);

			ayr_construct(T, &v, std::move(item));
			return v;
		}

		// 移除指定位置的元素
		void pop(c_size index = -1)
		{
			index = (index + size_) % size_;
			T& ret = __at__(index);
			ayr_destroy(&ret);

			for (c_size i = index; i < size_ - 1; ++i)
				__at__(i) = std::move(__at__(i + 1));

			if (all_one(--size_)) --occupies_size_;
		}

		// 转换为Array
		Array<T> to_array() const
		{
			Array<T> arr(size_);
			arr.fill(super::begin(), super::end());
			return arr;
		}

		// 容器的字符串形式
		CString __str__() const
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0; i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << __at__(i);
			}
			stream << "]";

			return CString(stream.str());
		}

		void release()
		{
			c_size last_block_size = size_;

			for (c_size i = 0; i < occupies_size_ - 1; ++i)
			{
				dynarray_[i].release(exp2(i));
				last_block_size -= exp2(i);
			}

			if (occupies_size_)
				dynarray_[occupies_size_ - 1].release(last_block_size);
			size_ = 0;
			occupies_size_ = 0;
		}
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

			ayr_construct(Array<T>, &block, Array<T>::noconstruct(exp2(occupies_size_ - 1)));
		}
	private:
		Array<Array<T>> dynarray_;

		c_size size_, occupies_size_;
	};
}