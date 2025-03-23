#include <ayr/Dict.hpp>
#include <ayr/Set.hpp>
#include <ayr/timer.hpp>
#include <ayr/Atring.hpp>

#include <unordered_map>
#include <unordered_set>

using namespace ayr;

void dict_run_speed_test()
{
	Timer_ms t;
	Dict<std::string, int> d;
	std::vector<std::string> vs;
	constexpr int N = 1e6;

	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	t.into();
	for (int i = 0; i < N; i++)
		d.insert(vs[i], i);
	print("Dict insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		d[vs[i]];
	print("Dict query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
	{
		d.pop(vs[i]);
	}

	print("Dict pop time: ", t.escape(), "ms");

	std::unordered_map<std::string, int> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(std::make_pair(vs[i], i));
	print("std::unordered_map insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u[vs[i]];
	print("std::unordered_map query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u.erase(vs[i]);
	print("std::unordered_map pop time: ", t.escape(), "ms");
}

void set_run_speed_test()
{
	Timer_ms t;
	std::vector<std::string> vs;
	constexpr int N = 1e6;
	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	Set<std::string> s;
	t.into();
	for (int i = 0; i < N; i++)
		s.insert(vs[i]);
	print("Set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(s.contains(vs[i]));
	print("Set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		s.pop(vs[i]);
	print("Set pop time: ", t.escape(), "ms");

	std::unordered_set<std::string> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(vs[i]);
	print("std::unordered_set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(u.count(vs[i]));
	print("std::unordered_set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u.erase(vs[i]);
	print("std::unordered_set pop time: ", t.escape(), "ms");
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
	print.setend(" ");
	for (auto& k : d.keys())
		print(k);
	print("\n");
	for (auto& v : d.values())
		print(v);
	print("\n");
	for (auto [k, v] : d.items())
		print(k, v);

	print.setend("\n");

	d.insert(5, 5);
	d.insert(6, 6);
	d.insert(7, 7);
	d.insert(8, 8);
	print(d.size());
	print("after insert {5, 5}, {6, 6}, {7, 7}, {8, 8}:", d, "\n");

	print("key 1 value: ", d.get(1));

	d.pop(1);
	d.pop(2);
	print(d.size());
	print("after pop 1, 2:", d, "\n");

	Dict<int, int> d2{ {9, 9} };
	d.update({ {10, 10} });
	d.update(d2);
	print(d.size());
	print("after update {9, 9}, {10, 10}:", d, "\n");

	d.clear();
	print(d.size());
	print("after clear:", d, "\n");
	d.items();
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

	set_run_speed_test();
}

int main()
{
	dict_test();
	set_test();
	return 0;
}