#ifndef AYR_BASE_INDEXITERATOR_HPP
#define AYR_BASE_INDEXITERATOR_HPP

#include "IteratorInfo.hpp"

namespace ayr
{
	// 索引容器迭代器,
	template<bool IsConst, typename C, typename V>
	class IndexIterator :
		public IteratorInfo<IndexIterator<IsConst, C, V>, add_const_t<IsConst, C>, std::random_access_iterator_tag, add_const_t<IsConst, V>>
	{
		using ItInfo = IteratorInfo<IndexIterator<IsConst, C, V>, add_const_t<IsConst, C>, std::random_access_iterator_tag, add_const_t<IsConst, V>>;

		using self = IndexIterator;
	public:
		IndexIterator() : container_(nullptr), index_(0) {}

		IndexIterator(ItInfo::container_type* container, c_size index) : container_(container), index_(index) {}

		IndexIterator(const self& other) : IndexIterator(other.container_, other.index_) {}

		self& operator=(const self& other) { container_ = other.container_; index_ = other.index_; return *this; }

		ItInfo::reference operator*() const { return container_->operator[](index_); }

		ItInfo::pointer operator->() const { return &container_->operator[](index_); }

		self& operator++() { ++index_; return *this; }

		self operator++(int) { self tmp(*this); ++index_; return tmp; }

		self& operator--() { --index_; return *this; }

		self operator--(int) { self tmp(*this); --index_; return tmp; }

		self operator+(ItInfo::difference_type n) const { return self(container_, index_ + n); }

		self operator-(ItInfo::difference_type n) const { return self(container_, index_ - n); }

		self& operator+=(ItInfo::difference_type n) { index_ += n; return *this; }

		self& operator-=(ItInfo::difference_type n) { index_ -= n; return *this; }

		ItInfo::difference_type operator-(const self& other) const { return index_ - other.index_; }

		bool __equals__(const self& other) const { return container_ == other.container_ && index_ == other.index_; }

	private:
		ItInfo::container_type* container_;

		c_size index_;
	};
}

#endif