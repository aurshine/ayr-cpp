#pragma once

#include <vector>
#include <string>
#include <ayr/Array.hpp>
#include <ayr/DynArray.hpp>
#include <ayr/timer.hpp>

using namespace ayr;

ayr::DynArray<std::string> das;
std::vector<std::string> vs;
ayr::DynArray<int> dai;
std::vector<int> vi;

constexpr int N = 1e6;

void test_das()
{
	print("test_das");
	for (int i = 0; i < N; ++i)
	{
		das.append("hello");
	}
}

void test_vs()
{
	print("test_vs");
	for (int i = 0; i < N; ++i)
	{
		vs.push_back("hello");
	}
}

void test_dai()
{
	print("test_dai");
	for (int i = 0; i < N; ++i)
	{
		dai.append(i);
	}
}

void test_vi()
{
	print("test_vi");
	for (int i = 0; i < N; ++i)
	{
		vi.push_back(i);
	}
}

void runspeed_test()
{
	auto tm = Timer("ms");

	tm(test_das);
	tm(test_vs);
	tm(test_dai);
	tm(test_vi);
}

void iterate_test()
{
	DynArray<int> da;
	for (int i = 0; i < 1e6; ++i)
		da.append(i);

	Timer tm("ms");
	tm.start();
	for (auto& i : da)
		i;
	tm.stop();

	tm.start();
	for (int i = 0, size = da.size(); i < size; ++i)
		da.at(i);

	tm.stop();
}

void dynarray_test()
{
	runspeed_test();

	DynArray<int> da;
	da.append(1);
	print(da[-1]);
	da.append(2);
	print(da[-1]);
	da.append(3);
	print(da[-1]);
	da.append(4);
	print(da[-1]);
	da.append(5);
	print(da[-1]);
	da.append(6);
	print(da[-1]);
	da.append(7);
	print(da[-1]);
	da.append(8);
	print(da[-1]);
	da.append(9);
	print(da[-1]);
	da.append(10);
	print(da[-1]);

	da.pop();
	print("popped -1:", da);
	da.pop(2);
	print("popped 2:", da);
	da.clear();
	print("cleared:", da);
	da.insert(0, 1);
	print("inserted 1 at 0:", da);
	da.insert(0, 2);
	print("inserted 2 at 0:", da);
	da.insert(0, 3);
	print("inserted 3 at 0:", da);
	da.insert(0, 4);
	print("inserted 4 at 0:", da);
	da.insert(0, 5);
	print("inserted 5 at 0:", da);
	da.insert(10, 6);
	print("inserted 6 at 10:", da);
	da.insert(10, 7);
	print("inserted 7 at 10:", da);

	da.clear();
	for (int i = 0; i < 64; ++i)
		da.append(i);
	print("appended 64:", da);
}
