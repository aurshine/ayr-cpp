#ifndef AYR_DYNAARRAY_HPP
#define AYR_DYNAARRAY_HPP

#include <utility>

#include <ayr/detail/bunit.hpp>
#include <ayr/detail/Buffer.hpp>
#include <ayr/Array.hpp>

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

		DynArray() : blocks_(DYNARRAY_BLOCK_SIZE, 0), occupies_size_(0) {}

		DynArray(const self& other) : DynArray()
		{
			for (auto& item : other)
				append(item);
		}

		DynArray(self&& other) : blocks_(std::move(other.blocks_)), occupies_size_(other.occupies_size_) {}

		~DynArray() {};

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			blocks_ = other.blocks_;
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other)
				return *this;

			blocks_ = std::move(other.blocks_);
			occupies_size_ = other.occupies_size_;
			return *this;
		}

		// 容器存储的数据长度
		c_size size() const
		{
			c_size size = 0;
			for (c_size i = 0; i < occupies_size_; ++i)
				size += blocks_.at(i).size();
			return size;
		}

		// 容器已经占用的块
		c_size occupy_size() const { return occupies_size_; }

		// 追加元素
		T& append(const T& item)
		{
			if (occupied_block_has_full())
				wakeup();

			return blocks_.at(occupies_size_ - 1).append(item);
		}

		T& append(T&& item)
		{
			if (occupied_block_has_full())
				wakeup();

			return blocks_.at(occupies_size_ - 1).append(std::move(item));
		}


		// 移除指定位置的元素
		void pop(c_size index = -1)
		{
			c_size size_ = size();
			index = neg_index(index, size_);

			for (c_size i = index + 1; i < size_; ++i)
				at(i - 1) = std::move(at(i));

			blocks_.at(occupies_size_ - 1).pop_back();
			if (occupied_block_has_full())
				pop_back_block();
		}

		// 转换为Array
		Array<T> to_array() const
		{
			c_size size_ = size();
			Array<T> arr(size_);
			for (c_size i = 0; i < size_; ++i)
				arr.at(i) = at(i);

			return arr;
		}

		// 移动数组
		Array<T> move_array()
		{
			c_size size_ = size();
			Array<T> arr(size_);
			for (c_size i = 0; i < size_; ++i)
				arr.at(i) = std::move(at(i));

			return arr;
		}

		// 容器的字符串形式
		CString __str__() const
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0, size_ = size(); i < size_; ++i)
			{
				if (i) stream << ", ";
				stream << at(i);
			}
			stream << "]";

			return CString(stream.str());
		}

		// 对index范围不做检查
		T& at(c_size index)
		{
			c_size block_index = _get_block_index(index);
			++index;
			return blocks_.at(block_index).at(index ^ exp2(block_index));
		}

		// 对index范围不做检查
		const T& at(c_size index) const
		{
			c_size block_index = _get_block_index(index);
			++index;
			return blocks_.at(block_index).at(index ^ exp2(block_index));
		}

		void clear()
		{
			for (auto&& block : blocks_)
				block.resize(0);
			occupies_size_ = 0;
		}
	private:
		// 移除最后一个块
		void pop_back_block()
		{
			blocks_.at(occupies_size_ - 1).resize(0);
			--occupies_size_;
		}


		// 存在的块是否都已经满了
		bool occupied_block_has_full() const
		{
			c_size size_ = size();
			return all_one(size_) || size_ == 0;
		}


		// 唤醒一个新的块
		void wakeup()
		{
			occupies_size_++;

			Buffer<T>& block = blocks_.at(occupies_size_ - 1);

			block.resize(exp2(occupies_size_ - 1));
		}

		// 得到index表示的块的索引
		static c_size _get_block_index(c_size index) { return highbit_index(index + 1); }
	private:
		Array<Buffer<T>> blocks_;

		c_size occupies_size_;
	};
}

#endif