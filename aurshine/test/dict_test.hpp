#pragma once
#include <ayr/Dict.hpp>
#include <ayr/Set.hpp>

#include <unordered_map>


using namespace ayr;

void dict_test()
{
	Dict<int, int> d{ {1, 1}, {2, 2}, {3, 3}, {4, 4	} };
	d.insert(5, 5);
	d.insert(6, 6);
	d.insert(7, 7);
	d.insert(8, 8);

	print(d.size());
	print(d);
	print("\n");
	d.pop(1);
	d.pop(2);

	print(d.size());
	print(d);

	Set<int> s{ 1, 2, 3, 4, 5, 6, 7, 8 };

	print(s.contains(1));
	s.pop(2);
	s.pop(3);

	print(s);
}

void dict_run_speed_test()
{
	Timer_ms t;
	Dict<std::string, int> d;
	t.into();
	for (int i = 0; i < 1e6; i++)
		d.insert(std::to_string(i), i);
	print("Dict insert time: ", t.escape());

	std::unordered_map<std::string, int> u;
	t.into();
	for (int i = 0; i < 1e6; i++)
		u.insert(std::make_pair(std::to_string(i), i));
	print("std::unordered_map insert time: ", t.escape());
}

void dict_and_or_xor_test()
{
	Dict<int, int> d1{ {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5} };
	Dict<int, int> d2{ {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7} };

	for (auto& value : d1.values())
		print("values: ", value);

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