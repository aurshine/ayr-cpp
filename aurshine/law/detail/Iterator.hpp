#ifndef AYR_LAW_DETAIL_ITERATOR_HPP
#define AYR_LAW_DETAIL_ITERATOR_HPP

#include <iterator>

#include <law/detail/Printer.hpp>
#include <law/detail/ayr_traits.hpp>

namespace ayr
{
	// 迭代器接口
	template<typename V, typename ChildIterator>
	class IteratorImpl : public Object<ChildIterator>
	{
		using super = Object<ChildIterator>;

		using self = IteratorImpl<V, ChildIterator>;

	public:
		using Value_t = V;

		using Distance_t = std::ptrdiff_t;

		using AyrIteratorDerived = ChildIterator;

		using AyrIteratorDerivedValue = V;

		IteratorImpl() = default;

		IteratorImpl(const self& other) = default;

		self& operator=(const self& other) = default;

		virtual ~IteratorImpl() = default;

		virtual Value_t& operator*() { NotImplementedError("Not implemented Value_t& operator*()"); return None<Value_t>; }

		virtual Value_t* operator->() { NotImplementedError("Not implemented Value_t& operator->()"); return &None<Value_t>; }

		virtual const Value_t& operator*() const { NotImplementedError("Not implemented const Value_t& operator*() const"); return None<Value_t>; }

		virtual const Value_t* operator->() const { NotImplementedError("Not implemented const Value_t& operator->() const"); return &None<Value_t>; }

		virtual ChildIterator& operator++() { NotImplementedError("Not implemented self& operator++()"); return None<ChildIterator>; }

		virtual ChildIterator& operator--() { NotImplementedError("Not implemented self& operator--()"); return None<ChildIterator>; }

		virtual bool __equals__(const ChildIterator& other) const { NotImplementedError("Not implemented bool __cmp__(const self& other) const"); return 0; }

		virtual Distance_t distance(const ChildIterator& other) const { NotImplementedError("Not implemented Distance_t __distance__(const self& other) const"); return 0; }
	};

	template<typename T>
	concept AyrIterator = requires(T & t)
	{
		typename T::AyrIteratorDerived;

		typename T::AyrIteratorDerivedValue;

	}&& isinstance<T, IteratorImpl<typename T::AyrIteratorDerivedValue, typename T::AyrIteratorDerived>>;


	template<typename T>
	constexpr bool is_ayr_iterator = AyrIterator<T>;
}
#endif