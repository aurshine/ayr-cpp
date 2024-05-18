#include "../aurshine/law/Iterator/IteratorExecutor.hpp"
#include <iostream>
#include <vector>


using namespace ayr;
constexpr size_t N = 1e7;

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
			if (i == N)
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
	auto start = clock();
	for (auto i : Seq());
	std::cout << "Time: " << (clock() - start) << std::endl;

	start = clock();
	for (auto i : std::vector<int>(N, 0));
	std::cout << "Time: " << (clock() - start) << std::endl;
}