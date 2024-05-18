#pragma once
#include "Iterator.hpp"

namespace ayr
{
	// 迭代器执行器
	template<class T>
	class IteratorExecutor : public Comparator<IteratorExecutor<T>>
	{
	public:
		constexpr IteratorExecutor(Iterator<T>* _it) :iter_executor(_it) {}

		virtual ~IteratorExecutor() = default;

	public:
		IteratorExecutor& operator++()
		{
			iter_executor = iter_executor->__next__();
			return *this;
		}

		IteratorExecutor& operator-- ()
		{
			iter_executor = iter_executor->__prev__();
			return *this;
		}

		T& operator*() { return iter_executor->__iterval__(); }

		T* operator->() { return &iter_executor->__iterval__(); }

		const T& operator*() const { return iter_executor->__iterval__(); }

		const T* operator->() const { return &iter_executor->__iterval__(); }

		bool __eq__(const IteratorExecutor<T>& other) const override { return iter_executor == other.iter_executor; }

		bool stop_iterator() const { return iter_executor == nullptr; }

	private:
		Iterator<T>* iter_executor;
	};
}