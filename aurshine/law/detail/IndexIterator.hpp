#ifndef AYR_LAW_DETAIL_INDEXITERATOR_HPP
#define AYR_LAW_DETAIL_INDEXITERATOR_HPP

#include <law/detail/Iterator.hpp>


namespace ayr
{
	// 索引容器迭代器,
	template<typename C, typename V>
	class IndexIterator : public IteratorImpl<V, IndexIterator<C, V>>
	{
		using super = IteratorImpl<C>;

		using self = IndexIterator<C, V>;
	public:
		using Container_t = C;

		using Value_t = V;

		IndexIterator(Container_t& container, c_size index) : container_(container), index_(index) {}

		Value_t& operator*() override { return container_[index_]; }

		Value_t* operator->() override { &return container_[index_]; }

		self operator++() override { return IndexIterator(container_, index_ + 1); }

		self operator--() override { return IndexIterator(container_, index_ - 1); }

		cmp_t __equal__(const self& other) const { return &conatiner_ == &other.container_ && index_ == other.index_; }

		CString __str__() const { return CString(std::format("IndexIterator<{}, {}>(index={})", dtype(C), dtype(V), index_)); }
	private:
		Container_t& container_;

		c_size index_;
	};


	// 常量索引容器迭代器
	// GetItem: 用于获取容器元素的接口
	// GetItem: 需要实现 static const V& getcitem(container, index) 和 static const V* getcptr(container, index) 方法
	template<typename C, typename V>
	class CIndexIterator : public IteratorImpl<V>
	{
		using super = IteratorImpl<C>;

		using self = CIndexIterator<C, V>;
	public:
		using Container_t = C;

		using Value_t = V;

		CIndexIterator(const Container_t& container, c_size index) : container_(container), index_(index) {}

		const Value_t& operator*() const override { return GetItem::getcitem(container_, index_); }

		const Value_t* operator->() const override { return GetItem::getcptr(container_, index_); }

		self operator++() override { return IndexIterator(container_, index_ + 1); }

		self operator--() override { return IndexIterator(container_, index_ - 1); }

		cmp_t __equal__(const self& other) const { return &conatiner_ == &other.container_ && index_ == other.index_; }

		CString __str__() const { return CString(std::format("IndexIterator<{}, {}>(index={})", dtype(C), dtype(V), index_)); }
	private:
		const Container_t& container_;

		c_size index_;
	};


	template<typename C, typename V>
	class IndexContainer : Object
	{
	public:
		using Iterator = IndexIterator<C, V>;

		using ConstIterator = CIndexIterator<C, V>;

	public:
		virtual c_size size() const = 0;

		virtual V& operator[] (c_size index) = 0;

		virtual const V& operator[] (c_size index) const = 0;

		Iterator begin() { return Iterator(*this, 0); }

		Iterator end() { return Iterator(*this, size()); }

		ConstIterator begin() const { return ConstIterator(*this, 0); }

		ConstIterator end() const { return ConstIterator(*this, size()); }
	};
}

#endif