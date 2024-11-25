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
	for (int i = 0; i < N; ++i)
	{
		das.append("hello");
	}
}

void test_vs()
{
	for (int i = 0; i < N; ++i)
	{
		vs.push_back("hello");
	}
}

void test_dai()
{
	for (int i = 0; i < N; ++i)
	{
		dai.append(i);
	}
}

void test_vi()
{
	for (int i = 0; i < N; ++i)
	{
		vi.push_back(i);
	}
}

void runspeed_test()
{
	Timer_ms tm;

	print("dynarray append str time:", tm(test_das), "ms");
	print("vector push_back str time:", tm(test_vs), "ms");
	print("dayarray append int time:", tm(test_dai), "ms");
	print("vector push_back int time:", tm(test_vi), "ms");
}

void iterate_test()
{
	DynArray<int> da;
	for (int i = 0; i < 1e6; ++i)
		da.append(i);

	Timer_ms tm;
	tm.into();
	for (auto& i : da)
		i;
	print("DynArray iterate time:", tm.escape());

	tm.into();
	for (int i = 0, size = da.size(); i < size; ++i)
		da.at(i);

	print("DynArray at time:", tm.escape());
}

void dynarray_test()
{
	runspeed_test();

	DynArray<int> da;
	print.setend("\n\n");
	tlog(da.append(1));
	tlog(da.append(2));
	tlog(da.append(3));
	tlog(da.append(4));
	tlog(da.append(5));
	tlog(da.append(6));
	tlog(da.append(7));
	tlog(da.append(8));
	tlog(da.append(9));
	tlog(da.append(10));

	da.pop();
	tlog(std::format("popped -1: {}", da));
	da.pop(2);
	tlog(std::format("popped 2: {}", da));
	da.clear();
	tlog(std::format("cleared: {}", da));
	da.insert(0, 1);
	tlog(std::format("inserted 1 at 0: {}", da));
	da.insert(0, 2);
	tlog(std::format("inserted 2 at 0: {}", da));
	da.insert(0, 3);
	tlog(std::format("inserted 3 at 0: {}", da));
	da.insert(0, 4);
	tlog(std::format("inserted 4 at 0: {}", da));
	da.insert(0, 5);
	tlog(std::format("inserted 5 at 0: {}", da));
	da.insert(10, 6);
	tlog(std::format("inserted 6 at 10: {}", da));
	da.insert(10, 7);
	tlog(std::format("inserted 7 at 10: {}", da));

	da.clear();

	da.extend(Range(1e6));
	for (int i = 0; i < 1e6; ++i)
		if (i != da[i])
			tlog(std::format("da[{}] == {}", i, da[i]));

	DynArray<int> da2({ 1, 2, 3, 4, 5 });
	tlog(DynArray<int>({ 1, 2, 3, 4, 5 }));
	DynArray<int> da3(da2);
	tlog(DynArray<int>(da2));
	tlog(da2 + da3);
	tlog(da2.extend(da3));
	tlog(da2.extend(DynArray<int>({ 6, 7, 8 })));
	tlog(da3.to_array());
	tlog(da3.move_array());
	tlog(da3);
	tlog(DynArray<int>(std::vector<int>{1, 2, 3, 4, 5}));
}
