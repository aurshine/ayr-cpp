#ifndef AYR_LAW_DETAIL_ITERATOR_HPP
#define AYR_LAW_DETAIL_ITERATOR_HPP

#include <iterator>

#include <law/detail/Printer.hpp>
#include <law/detail/ayr_traits.hpp>

namespace ayr
{
	// 迭代器接口
	template<typename V, typename Child>
	class IteratorImpl : public Object
	{
		using super = Object;

		using self = IteratorImpl<V>;

	public:
		using Value_t = V;

		IteratorImpl() = default;

		IteratorImpl(const self& other) = default;

		self& operator=(const self& other) = default;

		virtual ~IteratorImpl() = default;

		virtual Value_t& operator*() { NotImplementedError("Not implemented Value_t& operator*()"); return None<Value_t>; }

		virtual Value_t* operator->() { NotImplementedError("Not implemented Value_t& operator->()"); return &None<Value_t>; }

		virtual const Value_t& operator*() const { NotImplementedError("Not implemented const Value_t& operator*() const"); return None<Value_t>; }

		virtual const Value_t* operator->() const { NotImplementedError("Not implemented const Value_t& operator->() const"); return &None<Value_t>; }

		virtual Child operator++() { NotImplementedError("Not implemented self& operator++()"); return *this; }

		virtual Child operator--() { NotImplementedError("Not implemented self& operator--()"); return *this; }

		virtual cmp_t __equal__(const self& other) const { NotImplementedError("Not implemented bool __cmp__(const self& other) const"); return 0; }

		Child operator++(int)
		{
			self tmp = *this;
			++(*this);
			return tmp;
		}

		Child operator--(int)
		{
			self tmp = *this;
			--(*this);
			return tmp;
		}
	};


	template<typename V>
	class ZipIterator : IteratorImpl<V>
	{

	};
}
#endif