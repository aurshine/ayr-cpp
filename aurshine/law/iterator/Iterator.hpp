#pragma once
#include "../printer.hpp"

namespace ayr
{
	// 迭代器基类
	template<typename T>
	class Iterator : public Object
	{
	public:
		constexpr Iterator() {};

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

		T& operator*() { return this->__iterval__(); }

		T* operator->() { return &this->__iterval__(); }

		const T& operator*() const { return this->__iterval__(); }

		const T* operator->() const { return &this->__iterval__(); }

		Iterator* operator++ (int) = delete;
		Iterator* operator-- (int) = delete;
	};
}