#ifndef AYR_DYNARRAY_HPP
#define AYR_DYNARRAY_HPP

#include <utility>

#include "base/bunit.hpp"
#include "base/Appender.hpp"
#include "Array.hpp"

namespace ayr
{
	// 动态数组
	template<typename T>
	class DynArray : public Sequence<DynArray<T>, T>
	{
		using self = DynArray<T>;

		using super = Sequence<self, T>;

		// 动态数组块的数量
		constexpr static int DYNARRAY_BLOCK_SIZE = 63;

		// 最小块大小
		constexpr static c_size BASE_SIZE = 8;
	public:
		using Value_t = T;

		DynArray() : blocks_(DYNARRAY_BLOCK_SIZE, 0), size_(0), back_block_index_(-1) {}

		DynArray(std::initializer_list<Value_t>&& init) : DynArray()
		{
			for (auto& item : init)
				append(item);
		}

		DynArray(const self& other) : DynArray()
		{
			for (const Value_t& item : other)
				append(item);
		}

		DynArray(self&& other) noexcept :
			blocks_(std::move(other.blocks_)),
			size_(other.size_),
			back_block_index_(other.back_block_index_)
		{
			other.size_ = 0;
			other.back_block_index_ = -1;
		}

		template<IteratableU<T> Obj>
		DynArray(Obj&& other) : DynArray()
		{
			for (auto&& item : other)
				append(std::forward<decltype(item)>(item));
		}

		~DynArray() = default;

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			blocks_ = other.blocks_;
			size_ = other.size_;
			back_block_index_ = other.back_block_index_;

			return *this;
		}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			blocks_ = std::move(other.blocks_);
			size_ = other.size_;
			back_block_index_ = other.back_block_index_;

			other.size_ = 0;
			other.back_block_index_ = -1;
			return *this;
		}

		// 容器存储的数据长度
		c_size size() const { return size_; }

		// 对index范围不做检查
		T& at(c_size index)
		{
			int block_index = _get_block_index(index + 1);
			c_size inblock_index = _get_inblock_index(index + 1, block_index);
			return blocks_.at(block_index).at(inblock_index);
		}

		// 对index范围不做检查
		const T& at(c_size index) const
		{
			int block_index = _get_block_index(index + 1);
			c_size inblock_index = _get_inblock_index(index + 1, block_index);
			return blocks_.at(block_index).at(inblock_index);
		}

		// 追加元素
		template<typename U>
		T& append(U&& item)
		{
			try_wakeup();

			T& res = _back_block().append(std::forward<U>(item));
			++size_;
			return res;
		}

		template<typename U>
		void insert(c_size index, U&& item)
		{
			append(std::forward<U>(item));
			for (c_size i = size() - 1; i > index; --i)
				std::swap(at(i), at(i - 1));
		}

		// 移除指定位置的元素
		void pop(c_size index = -1)
		{
			c_size m_size = size();
			index = neg_index(index, m_size);

			for (c_size i = index + 1; i < m_size; ++i)
				at(i - 1) = std::move(at(i));

			Appender<T>& back_block = _back_block();
			if (back_block.size() == 1)
				_pop_back_block();
			else
				back_block.pop_back();
			--size_;
		}

		void clear()
		{
			for (auto&& block : blocks_)
				block.resize(0);
			size_ = 0;
			back_block_index_ = -1;
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
			clear();
			return arr;
		}

		template<IteratableU<T> Obj>
		self& extend(Obj&& other)
		{
			for (auto&& item : other)
				append(std::forward<decltype(item)>(item));
			return *this;
		}

		self operator+ (const self& other) const { return self(*this).extend(other); }

		self operator+ (self&& other) const { return self(*this).extend(std::move(other)); }

		self& operator+= (const self& other) { return extend(other); }

		self& operator+= (self&& other) { return extend(std::move(other)); }

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

		template<bool IsConst>
		struct _Iterator : public IteratorInfo<_Iterator<IsConst>, add_const_t<IsConst, DynArray<Value_t>>, std::random_access_iterator_tag, add_const_t<IsConst, Value_t>>
		{
			using self = _Iterator<IsConst>;

			using super = IteratorInfo<_Iterator<IsConst>, add_const_t<IsConst, DynArray<Value_t>>, std::random_access_iterator_tag, add_const_t<IsConst, Value_t>>;

			_Iterator() : _Iterator(nullptr) {}

			_Iterator(super::container_type* dynarray) : _Iterator(dynarray, 0, 0) {}

			_Iterator(super::container_type* dynarray, c_size block_index, c_size inblock_index)
				: block_index_(block_index), inblock_index_(inblock_index), dynarray_(dynarray) {}

			_Iterator(const self& other) : _Iterator(other.dynarray_, other.block_index_, other.inblock_index_) {}

			self& operator=(const self& other)
			{
				block_index_ = other.block_index_;
				inblock_index_ = other.inblock_index_;
				dynarray_ = other.dynarray_;
				return *this;
			}

			super::reference operator*() const { return dynarray_->blocks_.at(block_index_).at(inblock_index_); }

			super::pointer operator->() const { return &dynarray_->blocks_.at(block_index_).at(inblock_index_); }

			self& operator++()
			{
				if (dynarray_->blocks_.at(block_index_).size() == inblock_index_ + 1)
				{
					++block_index_;
					inblock_index_ = 0;
				}
				else
					++inblock_index_;
				return *this;
			}

			self operator++(int) { self tmp(*this); ++tmp; return tmp; }

			self& operator--()
			{
				if (inblock_index_ == 0)
				{
					--block_index_;
					inblock_index_ = dynarray_->blocks_.at(block_index_).size() - 1;
				}
				else
					--inblock_index_;
				return *this;
			}

			self operator--(int) { self tmp(*this); --tmp; return tmp; }

			self operator+(super::difference_type n) const { self tmp(*this); tmp += n; return tmp; }

			self operator-(super::difference_type n) const { self tmp(*this); tmp -= n; return tmp; }

			self& operator+=(super::difference_type n)
			{
				while (n)
				{
					c_size cur_size = dynarray_->blocks_.at(block_index_).size();
					if (cur_size > inblock_index_ + n)
					{
						inblock_index_ += n;
						n = 0;
					}
					else
					{
						inblock_index_ = 0;
						n -= cur_size - inblock_index_;
						block_index_ += 1;
					}
				}
				return *this;
			}

			self& operator-=(super::difference_type n)
			{
				while (n)
				{
					if (inblock_index_ >= n)
						inblock_index_ -= n;
					else
					{
						n -= inblock_index_;
						--block_index_;
						inblock_index_ = dynarray_->blocks_.at(block_index_).size() - 1;
					}
				}
				return *this;
			}

			super::difference_type operator-(const self& other) const
			{
				if (*this < other) return -(other - *this);
				if (dynarray_ != other.dynarray_)
					RuntimeError("Iterator not belong to the same container");

				if (block_index_ == other.block_index_)
					return inblock_index_ - other.inblock_index_;

				c_size res = dynarray_->blocks_.at(other.block_index_).size() - other.inblock_index_;

				for (c_size i = other.block_index_ + 1; i < block_index_; ++i)
					res += dynarray_->blocks_.at(i).size();

				return res + inblock_index_;
			}

			bool __equals__(const self& other) const
			{
				return block_index_ == other.block_index_ && inblock_index_ == other.inblock_index_ && dynarray_ == other.dynarray_;
			}

			cmp_t __cmp__(const self& other) const
			{
				if (block_index_ != other.block_index_)
					return block_index_ - other.block_index_;
				else
					return inblock_index_ - other.inblock_index_;
			}
		private:
			c_size block_index_, inblock_index_;

			super::container_type* dynarray_;
		};

		using Iterator = _Iterator<false>;

		using ConstIterator = _Iterator<true>;

		Iterator begin() { return Iterator(this); }

		ConstIterator begin() const { return ConstIterator(this); }

		Iterator end() { return Iterator(this, _back_block_index() + 1, 0); }

		ConstIterator end() const { return ConstIterator(this, _back_block_index() + 1, 0); }
	private:
		// 得到第ith个元素的块索引
		int _get_block_index(c_size ith) const
		{
			int i = 0, j = DYNARRAY_BLOCK_SIZE, mid;
			while (i < j)
			{
				mid = i + j >> 1;
				if (ith <= (exp2[mid + 1] - 1) * BASE_SIZE)
					j = mid;
				else
					i = mid + 1;
			}
			return i;
		}

		// 得到第ith个元素的块内索引
		c_size _get_inblock_index(c_size ith, int block_index) const
		{
			return ith - BASE_SIZE * (exp2[block_index] - 1) - 1;
		}

		// 最后一个块的索引
		int _back_block_index() const { return back_block_index_; }

		// 最后一个块
		Appender<T>& _back_block() { return blocks_.at(_back_block_index()); }

		// 移除最后一个块
		void _pop_back_block() { _back_block().resize(0); --back_block_index_; }

		// 尝试唤醒一个新的块
		void try_wakeup()
		{
			c_size bbi = _back_block_index();
			if (bbi == -1 || blocks_.at(bbi).full())
			{
				c_size new_size = ifelse(bbi == -1, BASE_SIZE, blocks_.at(bbi).size() * 2);

				++back_block_index_;
				_back_block().resize(new_size);
			}
		}
	private:
		Array<Appender<T>> blocks_;

		c_size size_;

		int back_block_index_;
	};
}

#endif