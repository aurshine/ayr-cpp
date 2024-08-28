#ifndef AYR_LAW_DETAIL_INDEXITERATOR_HPP
#define AYR_LAW_DETAIL_INDEXITERATOR_HPP

#include <law/detail/Iterator.hpp>


namespace ayr
{
	// Ë÷ÒýÈÝÆ÷µü´úÆ÷,
	template<typename C, typename V>
	class IndexIterator : public IteratorImpl<V, IndexIterator<C, V>>
	{
		using super = IteratorImpl<C, IndexIterator<C, V>>;

		using self = IndexIterator;
	public:
		using Container_t = C;

		using Value_t = V;

		IndexIterator(Container_t& container, c_size index) : container_(container), index_(index) {}

		IndexIterator(const self& other): IndexIterator(other.container_, other.index_) {}

		Value_t& operator*() override { return container_[index_]; }

		Value_t* operator->() override { return &container_[index_]; }

		self& operator++() override { ++index_; return *this; }

		self& operator--() override { --index_; return *this; }

		cmp_t __equal__(const self& other) const { return &conatiner_ == &other.container_ && index_ == other.index_; }

		CString __str__() const { return CString(std::format("IndexIterator<{}, {}>(index={})", dtype(C), dtype(V), index_)); }
	private:
		Container_t& container_;

		c_size index_;
	};


	// ³£Á¿Ë÷ÒýÈÝÆ÷µü´úÆ÷
	template<typename C, typename V>
	class CIndexIterator : public IteratorImpl<V, CIndexIterator<C, V>>
	{
		using super = IteratorImpl<C, CIndexIterator<C, V>>;

		using self = CIndexIterator<C, V>;
	public:
		using Container_t = C;

		using Value_t = V;

		CIndexIterator(const Container_t& container, c_size index) : container_(container), index_(index) {}

		CIndexIterator(const self& other) : CIndexIterator(other.container_, other.index_) {}

		const Value_t& operator*() const override { return container_[index_]; }

		const Value_t* operator->() const override { return &container_[index_]; }

		self& operator++() override { ++index_; return *this; }

		self& operator--() override { --index_; return *this; }

		cmp_t __equal__(const self& other) const { return (&conatiner_ == &other.container_) && (index_ == other.index_); }

		CString __str__() const { return CString(std::format("CIndexIterator<{}, {}>(index={})", dtype(C), dtype(V), index_)); }
	private:
		const Container_t& container_;

		c_size index_;
	};


	template<typename V>
	class IndexContainer : Object
	{
		using self = IndexContainer<V>;
	public:
		using Iterator = IndexIterator<self, V>;

		using ConstIterator = CIndexIterator<self, V>;

	public:
		virtual c_size size() const = 0;

		virtual V& operator[] (c_size index) = 0;

		virtual const V& operator[] (c_size index) const = 0;

		virtual Iterator begin() { return Iterator(*this, 0); }

		virtual Iterator end() { return Iterator(*this, size()); }

		virtual ConstIterator begin() const { return ConstIterator(*this, 0); }

		virtual ConstIterator end() const { return ConstIterator(*this, size()); }
	};
}

#endif