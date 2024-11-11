#pragma once

#include <vector>
#include <string>
#include <ayr/Array.hpp>
#include <ayr/DynArray.hpp>
#include <ayr/timer.hpp>

ayr::DynArray<int> das;
std::vector<int> vs;

constexpr int N = 1e6;

void test_das()
{
	ayr::print("test_das");
	for (int i = 0; i < N; ++i)
	{
		das.append(1);
	}
}

void test_vs()
{
	ayr::print("test_vs");
	for (int i = 0; i < N; ++i)
	{
		vs.push_back(1);
	}
}

void dynarray_test()
{
	auto tm = ayr::Timer("ms");

	tm(test_das);
	tm(test_vs);
}

/*
输出:
test_das
pass time: 723.38 ms
test_vs
pass time: 2700.41 ms
*/