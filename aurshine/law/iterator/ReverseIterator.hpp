#pragma once
#include "Iterator.hpp"

namespace ayr
{
	// 反向迭代器
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