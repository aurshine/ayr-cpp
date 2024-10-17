#ifndef AYR_LAW_DETAIL_INDEXITERATOR_HPP
#define AYR_LAW_DETAIL_INDEXITERATOR_HPP

#include <law/detail/Object.hpp>

namespace ayr
{
	// 索引容器迭代器,
	template<typename C, typename V>
	class IndexIterator : public Object<IndexIterator<C, V>>
	{
		using super = Object<IndexIterator>;

		using self = IndexIterator;
	public:
		using Container_t = C;

		using Value_t = V;

		IndexIterator(Container_t& container, c_size index) : container_(container), index_(index) {}

		IndexIterator(const self& other) : IndexIterator(other.container_, other.index_) {}

		Value_t& operator*() { return container_[index_]; }

		const Value_t& operator*() const { return container_[index_]; }

		Value_t* operator->() { return &container_[index_]; }

		const Value_t* operator->() const { return &container_[index_]; }

		self& operator++() { ++index_; return *this; }

		self operator++(int) { self tmp(*this); ++index_; return tmp; }

		self& operator--() { --index_; return *this; }

		self operator--(int) { self tmp(*this); --index_; return tmp; }

		bool __equals__(const self& other) const override { return (&container_ == &other.container_) && (index_ == other.index_); }

		c_size distance(const self& other) const { return std::abs(other.index_ - index_); }

	private:
		Container_t& container_;

		c_size index_;
	};


	// 常量索引容器迭代器
	template<typename C, typename V>
	class CIndexIterator : public Object<CIndexIterator<C, V>>
	{
		using self = CIndexIterator<C, V>;

		using super = Object<self>;
	public:
		using Container_t = C;

		using Value_t = V;

		CIndexIterator(const Container_t& container, c_size index) : container_(container), index_(index) {}

		CIndexIterator(const self& other) : CIndexIterator(other.container_, other.index_) {}

		const Value_t& operator*() const { return container_[index_]; }

		const Value_t* operator->() const { return &container_[index_]; }

		self& operator++() { ++index_; return *this; }

		self operator++(int) { self tmp(*this); ++index_; return tmp; }

		self& operator--() { --index_; return *this; }

		self operator--(int) { self tmp(*this); --index_; return tmp; }

		bool __equals__(const self& other) const override { return (&container_ == &other.container_) && (index_ == other.index_); }

		c_size distance(const self& other) const { return std::abs(other.index_ - index_); }
	private:
		const Container_t& container_;

		c_size index_;
	};
}

#endif