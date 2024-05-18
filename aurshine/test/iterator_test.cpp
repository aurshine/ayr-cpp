#include "../law/Iterator/IteratorExecutor.hpp"
#include <iostream>


using namespace ayr;

class Seq
{
public:
	class SeqIterator : public ayr::Iterator<int>
	{
	public:
		SeqIterator() : i(0) {}
		SeqIterator(int _i) : i(_i) {}

		Iterator<int>* __prev__() override
		{
			if (i == 0)
				return nullptr;

			this->i--;
			return this;
		}

		Iterator<int>* __next__() override
		{
			if (i == 10)
				return nullptr;

			this->i++;
			return this;
		}

		int& __iterval__() override
		{
			return i;
		}
		int i;
	};

	IteratorExecutor<int> begin()
	{
		return IteratorExecutor<int>(new SeqIterator(0));
	}

	IteratorExecutor<int> end()
	{
		return IteratorExecutor<int>(nullptr);
	}
};

inline void iterator_test()
{
	Seq seq;
	for (auto i : seq)
	{
		std::cout << i << std::endl;
	}
}