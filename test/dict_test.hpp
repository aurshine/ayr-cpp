#pragma once
#include <ayr/Dict.hpp>
#include <ayr/Set.hpp>

#include <unordered_map>


using namespace ayr;

void dict_run_speed_test()
{
	Timer_ms t;
	Dict<std::string, int> d;
	std::vector<std::string> vs;
	for (int i = 0; i < 1e6; i++)
		vs.push_back(std::to_string(i));

	t.into();
	for (int i = 0; i < 1e6; i++)
		d.insert(vs[i], i);
	print("Dict insert time: ", t.escape(), "ms");

	std::unordered_map<std::string, int> u;
	t.into();
	for (int i = 0; i < 1e6; i++)
		u.insert(std::make_pair(vs[i], i));
	print("std::unordered_map insert time: ", t.escape(), "ms");
}

void dict_and_or_xor_test()
{
	Dict<int, int> d1{ {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5} };
	Dict<int, int> d2{ {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7} };

	print("d1: ", d1);
	print("d2: ", d2);

	print("d1 & d2: ", d1 & d2);
	print("d1 | d2: ", d1 | d2);
	print("d1 ^ d2: ", d1 ^ d2);

	print("d1 &= d2: ", d1 &= d2);
	print("d1 |= d2: ", d1 |= d2);
	print("d1 ^= d2: ", d1 ^= d2);
}

void set_and_or_xor_test()
{
	Set<int> s1{ 1, 2, 3, 4, 5 };
	Set<int> s2{ 3, 4, 5, 6, 7 };

	print("s1: ", s1);
	print("s2: ", s2);

	print("s1 & s2: ", s1 & s2);
	print("s1 | s2: ", s1 | s2);
	print("s1 ^ s2: ", s1 ^ s2);

	print("s1 &= s2: ", s1 &= s2);
	print("s1 |= s2: ", s1 |= s2);
	print("s1 ^= s2: ", s1 ^= s2);
}

void dict_test()
{
	Dict<int, int> d{ {1, 1}, {2, 2}, {3, 3}, {4, 4	} };
	print(d.size());
	print("d:", d, "\n");

	d.insert(5, 5);
	d.insert(6, 6);
	d.insert(7, 7);
	d.insert(8, 8);
	print(d.size());
	print("after insert {5, 5}, {6, 6}, {7, 7}, {8, 8}:", d, "\n");

	d.pop(1);
	d.pop(2);
	print(d.size());
	print("after pop 1, 2:", d, "\n");

	Dict<int, int> d2{ {9, 9} };
	d.update({ {10, 10} });
	d.update(d2);
	print(d.size());
	print("after update {10, 10}:", d, "\n");

	d.clear();
	print(d.size());
	print("after clear:", d, "\n");
	dict_run_speed_test();
	dict_and_or_xor_test();
}

void set_test()
{
	Set<int> s{ 1, 2, 3, 4, 5 };
	print(s.size());
	print("s:", s, "\n");

	s.insert(6);
	s.insert(7);
	s.insert(8);
	print(s.size());
	print("after insert 6, 7, 8:", s, "\n");

	s.pop(1);
	s.pop(2);
	print(s.size());
	print("after pop 1, 2:", s, "\n");

	set_and_or_xor_test();

	Array<int> a{ 1, 2 ,3, 4 ,5 };
	print(set<int>(a));
}