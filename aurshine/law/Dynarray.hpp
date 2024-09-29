#ifndef AYR_LAW_DYNAARRAY_HPP
#define AYR_LAW_DYNAARRAY_HPP

#include <utility>

#include <law/detail/bunit.hpp>
#include <law/detail/Buffer.hpp>
#include <law/Array.hpp>

namespace ayr
{
	// 动态数组
	template<typename T>
	class DynArray : public Sequence<T>
	{
		using self = DynArray<T>;

		using super = Sequence<T>;

		// 动态数组块的数量
		constexpr static size_t DYNARRAY_BLOCK_SIZE = 64;
	public:
		using Value_t = T;

		DynArray() : dynarray_(DYNARRAY_BLOCK_SIZE, 0), size_(0), occupies_size_(0) {}

		DynArray(const self& other) : DynArray()
		{
			for (auto& item : other)
				append(item);
		}


		DynArray(self&& other) : dynarray_(std::move(other.dynarray_)), size_(other.size_), occupies_size_(other.occupies_size_) {}

		~DynArray() = default;

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

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

		// 容器存储的数据长度
		c_size size() const { return size_; }

		// 容器已经占用的块
		c_size occupy_size() const { return occupies_size_; }

		// 追加元素
		T& append(const T& item)
		{
			if (occupied_block_has_full())
				wakeup();

			++size_;
			return dynarray_[occupies_size_ - 1].append(item);
		}

		T& append(T&& item)
		{
			if (occupied_block_has_full())
				wakeup();

			++size_;
			return dynarray_[occupies_size_ - 1].append(item);
		}

		// 移除指定位置的元素
		void pop(c_size index = -1)
		{
			index = neg_index(index, size_);

			for (c_size i = index; i < size_ - 1; ++i)
				__at__(i) = std::move(__at__(i + 1));

			--size_;
			if (occupied_block_has_full() || size_ == 0)
				--occupies_size_;
		}

		// 转换为Array
		Array<T> to_array() const
		{
			Array<T> arr(size_);
			for (c_size i = 0; i < size_; ++i)
				ayr_construct(&arr[i], __at__(i));
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

	private:
		// 对index范围不做检查
		T& __at__(c_size index)
		{
			c_size l = _get_block_index(index);
			++index;
			return dynarray_[l][index ^ exp2(l)];
		}

		// 对index范围不做检查
		const T& __at__(c_size index) const
		{
			c_size l = _get_block_index(index);
			++index;
			return dynarray_[l][index ^ exp2(l)];
		}

		// 存在的块是否都已经满了
		bool occupied_block_has_full() const { return all_one(size_) || size_ == 0; }


		// 唤醒一个新的块
		void wakeup()
		{
			occupies_size_++;

			auto&& block = dynarray_.__at__(occupies_size_ - 1);

			ayr_construct(&block, exp2(occupies_size_ - 1));
		}

	private:
		// 得到index表示的块的索引
		static c_size _get_block_index(c_size index) { return highbit_index(index + 1); }
	private:
		Array<Buffer<T>> dynarray_;

		c_size size_, occupies_size_;
	};
}

#endif