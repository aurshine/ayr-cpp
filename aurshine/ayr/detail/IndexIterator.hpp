#ifndef AYR_DETAIL_INDEXITERATOR_HPP
#define AYR_DETAIL_INDEXITERATOR_HPP

#include <ayr/detail/Object.hpp>

namespace ayr
{
	// 索引容器迭代器,
	template<bool IsConst, typename C, typename V>
	class IndexIterator : public Object<IndexIterator<IsConst, C, V>>
	{
		using super = Object<IndexIterator>;

		using self = IndexIterator;
	public:
		using Container_t = std::conditional_t<IsConst, const C, C>;

		using iterator_category = std::random_access_iterator_tag;

		using value_type = std::conditional_t<IsConst, const V, V>;

		using difference_type = std::ptrdiff_t;

		using pointer = value_type*;

		using const_pointer = const value_type*;

		using reference = value_type&;

		using const_reference = const value_type&;

		IndexIterator() : container_(nullptr), index_(0) {}

		IndexIterator(Container_t* container, c_size index) : container_(container), index_(index) {}

		IndexIterator(const self& other) : IndexIterator(other.container_, other.index_) {}

		self& operator=(const self& other) { container_ = other.container_; index_ = other.index_; return *this; }

		reference operator*() { return container_->operator[](index_); }

		const_reference operator*() const { return container_->operator[](index_); }

		pointer operator->() { return &container_->operator[](index_); }

		const_pointer operator->() const { return &container_->operator[](index_); }

		self& operator++() { ++index_; return *this; }

		self operator++(int) { self tmp(*this); ++index_; return tmp; }

		self& operator--() { --index_; return *this; }

		self operator--(int) { self tmp(*this); --index_; return tmp; }

		self operator+(difference_type n) const { return self(container_, index_ + n); }

		self operator-(difference_type n) const { return self(container_, index_ - n); }

		self& operator+=(difference_type n) { index_ += n; return *this; }

		self& operator-=(difference_type n) { index_ -= n; return *this; }

		difference_type operator-(const self& other) const { return index_ - other.index_; }

		bool __equals__(const self& other) const override { return container_ == other.container_ && index_ == other.index_; }

	private:
		Container_t* container_;

		c_size index_;
	};
}

#endif