#pragma once

#include "printer.hpp"
#include "comparator.hpp"

namespace ayr
{
	template<typename T>
	class Iterator : public Comparator<Iterator<T>>
	{
	public:
		Iterator() = default;
		virtual ~Iterator() = default;

	public:
		// 当前迭代器的下一个迭代器
		virtual Iterator* __next__()
		{
			error_assert(false, "Iterator::__next__() not implemented");
			return nullptr;
		}

		// 当前迭代器的上一个迭代器
		virtual Iterator* __prev__()
		{
			error_assert(false, "Iterator::__prev__() not implemented");
			return nullptr;
		}

		// 当前迭代器指向的元素
		virtual T& __iterval__()
		{
			error_assert(false, "Iterator::__iterval__() not implemented");
			return *reinterpret_cast<T*>(nullptr);
		}

		Iterator* operator++()
		{
			this->__next__();
			return this;
		}

		Iterator* operator--()
		{
			this->__prev__();
			return this;
		}

		T& operator*() const
		{
			return this->__iterval__();
		}

		T* operator->() const
		{
			return &this->__iterval__();
		}

		Iterator* operator++ (int) = delete;
		Iterator* operator-- (int) = delete;
	};

	template<typename T>
	class ReverseIterator : public Iterator<T>
	{
	public:
		ReverseIterator(Iterator<T> _it) :it(_it) {}

		virtual ~ReverseIterator() = default;

	public:
		Iterator<T>* __next__() override
		{
			--this->it;
			return this;
		}

		Iterator<T>* __prev__() override
		{
			++this->it;
			return this;
		}

		T& __iterval__() override
		{
			return this->it.__iterval__();
		}
	private:
		Iterator<T> it;
	};
}