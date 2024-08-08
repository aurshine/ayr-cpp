#ifndef AYR_LAW_DETAIL_INDEXITERATOR_HPP
#define AYR_LAW_DETAIL_INDEXITERATOR_HPP

#include <law/detail/Iterator.hpp>


namespace ayr
{
	template<typename C, typename V>
	class GetIemImpl: Object
	{
	public:
		static V& getitem(C& container, size_t index) { return container[index]; }

		static V* getptr(C& container, size_t index) { return &container[index]; }

		static const V& getcitem(const C& container, size_t index) { return container[index]; }

		static const V* getcptr(const C& container, size_t index) { return &container[index]; }
	};


	// 索引容器迭代器,
	// GetItem: 用于获取容器元素的接口
	// GetItem: 需要实现 static V& getitem(container, index) 和 static V* getptr(container, index) 方法
	template<typename C, typename V, typename GetItem = GetIemImpl<C, V>>
	class IndexIterator : public IteratorImpl<V>
	{
		using super = IteratorImpl<C>;

		using self = IndexIterator<C, V, GetItem>;
	public:
		using Container_t = C;

		using Value_t = V;

		IndexIterator(Container_t& container, c_size index) : container_(container), index_(index) {}

		Value_t& operator*() override { return GetItem::getitem(container_, index_); }

		Value_t* operator->() override { return GetItem::getptr(container_, index_); }

		cmp_t __cmp__(const self& other) const { return index_ - other.index_; }

		self& operator++() override { ++index_; return *this; }

		self& operator--() override { --index_; return *this; }

		CString __str__() const { return CString(std::format("IndexIterator<{}, {}, {}>(index={})", dtype(C), dtype(V), dtype(GetItem), index_)); }
	private:
		Container_t& container_;

		c_size index_;
	};


	// 常量索引容器迭代器
	// GetItem: 用于获取容器元素的接口
	// GetItem: 需要实现 static const V& getcitem(container, index) 和 static const V* getcptr(container, index) 方法
	template<typename C, typename V, typename GetItem = GetIemImpl<C, V>>
	class CIndexIterator : public IteratorImpl<V>
	{
		using super = IteratorImpl<C>;

		using self = CIndexIterator<C, V, GetItem>;
	public:
		using Container_t = C;

		using Value_t = V;

		CIndexIterator(const Container_t& container, c_size index) : container_(container), index_(index) {}

		const Value_t& operator*() const override { return GetItem::getcitem(container_, index_); }

		const Value_t* operator->() const override { return GetItem::getcptr(container_, index_); }

		bool __cmp__(const self& other) const { return index_ != other.index_; }

		self& operator++() override { ++index_; return *this; }

		self& operator--() override { --index_; return *this; }

		CString __str__() const { return CString(std::format("IndexIterator<{}, {}, {}>(index={})", dtype(C), dtype(V), dtype(GetItem), index_)); }
	private:
		const Container_t& container_;

		c_size index_;
	};


	template<typename C, typename V, typename GetItem = GetIemImpl<C, V>>
	class IndexContainer : Object
	{
	public:
		using Iterator = IndexIterator<C, V, GetItem>;

		using ConstIterator = CIndexIterator<C, V, GetItem>;

	public:
		virtual c_size size() const { NotImplementedError("size() not implemented"); return 0; }

		virtual C& __iter_container__() const { NotImplementedError("__iter_container__() not implemented"); return None<C>; }

		Iterator begin() { return Iterator(__iter_container__(), 0); }

		Iterator end() { return Iterator(__iter_container__(), size()); }

		ConstIterator begin() const { return ConstIterator(__iter_container__(), 0); }

		ConstIterator end() const { return ConstIterator(__iter_container__(), size()); }
	};
}

#endif