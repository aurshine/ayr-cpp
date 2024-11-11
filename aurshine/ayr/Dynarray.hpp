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
	class DynArray : public Sequence<DynArray<T>, T>
	{
		using self = DynArray<T>;

		using super = Sequence<self, T>;

		// 动态数组块的数量
		constexpr static size_t DYNARRAY_BLOCK_SIZE = 64;

		// 最小块大小
		constexpr static int BASE_SIZE = 8;
	public:
		using Value_t = T;

		DynArray() : blocks_(DYNARRAY_BLOCK_SIZE, 0), size_(0) {}

		DynArray(const self& other) : DynArray()
		{
			for (const Value_t& item : other)
				append(item);
		}

		DynArray(self&& other) :blocks_(std::move(other.blocks_)), size_(other.size_)
		{
			other.size_ = 0;
		}

		~DynArray() {};

		self& operator=(const self& other)
		{
			if (this == &other)
				return *this;

			blocks_ = other.blocks_;
			size_ = other.size_;
			return *this;
		}

		self& operator=(self&& other)
		{
			if (this == &other)
				return *this;

			blocks_ = std::move(other.blocks_);
			size_ = other.size_;
			other.size_ = 0;
			return *this;
		}

		// 容器存储的数据长度
		c_size size() const { return size_; }

		// 追加元素
		template<typename U>
		T& append(U&& item)
		{
			++size_;
			try_wakeup();

			return _back_block().append(std::forward<U>(item));
		}

		// 移除指定位置的元素
		void pop(c_size index = -1)
		{
			c_size m_size = size();
			index = neg_index(index, m_size);

			for (c_size i = index + 1; i < m_size; ++i)
				at(i - 1) = std::move(at(i));

			Buffer<T>& back_block = _back_block();
			if (back_block.size() == 1)
				_pop_back_block();
			else
				back_block.pop_back();
			--size_;
		}

		template<typename U>
		void insert(c_size index, U&& item)
		{
			append(std::forward<U>(item));
			for (c_size i = size() - 1; i > index; --i)
				std::swap(at(i), at(i - 1));
		}

		// 转换为Array
		Array<T> to_array() const
		{
			c_size m_size = size();
			Array<T> arr(m_size);
			for (c_size i = 0; i < m_size; ++i)
				arr.at(i) = at(i);

			return arr;
		}

		// 移动数组
		Array<T> move_array()
		{
			c_size m_size = size();
			Array<T> arr(m_size);
			for (c_size i = 0; i < m_size; ++i)
				arr.at(i) = std::move(at(i));

			return arr;
		}

		// 容器的字符串形式
		CString __str__() const
		{
			std::stringstream stream;
			stream << "[";
			for (c_size i = 0, m_size = size(); i < m_size; ++i)
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
			int block_index = _get_block_index(index + 1);
			int inblock_index = _get_inblock_index(index + 1, block_index);
			return blocks_.at(block_index).at(inblock_index);
		}

		// 对index范围不做检查
		const T& at(c_size index) const
		{
			int block_index = _get_block_index(index + 1);
			int inblock_index = _get_inblock_index(index + 1, block_index);
			return blocks_.at(block_index).at(inblock_index);
		}

		void clear()
		{
			for (auto&& block : blocks_)
				block.resize(0);
			size_ = 0;
		}
	private:
		// 得到第ith个元素的块索引
		int _get_block_index(c_size ith) const
		{
			int i = 0, j = DYNARRAY_BLOCK_SIZE - 1, mid;
			while (i < j)
			{
				mid = i + j >> 1;
				if (ith <= exp2(mid) * BASE_SIZE)
					j = mid;
				else
					i = mid + 1;
			}
			return i;
		}

		// 得到第ith个元素的块内索引
		int _get_inblock_index(c_size ith, int block_index) const
		{
			return ith - BASE_SIZE * (exp2(block_index) - 1) - 1;
		}

		// 最后一个块的索引
		int _back_block_index() const { return _get_block_index(size()); }

		// 最后一个块
		Buffer<T>& _back_block() { return blocks_.at(_back_block_index()); }

		// 移除最后一个块
		void _pop_back_block() { _back_block().resize(0); }

		// 唤醒一个新的块
		// 此时的size一定要大于0
		void try_wakeup()
		{
			int back_block_index = _back_block_index();
			Buffer<T>& back_block = blocks_.at(back_block_index);
			if (back_block.size() == 0)
			{
				c_size new_size = BASE_SIZE;
				if (back_block_index != 0)
					new_size = blocks_.at(back_block_index - 1).size() * 2;

				back_block.resize(new_size);
			}
		}
	private:
		Array<Buffer<T>> blocks_;

		c_size size_;
	};
}

#endif