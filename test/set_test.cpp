#include <unordered_set>
#include <ayr/air/Set.hpp>

using namespace ayr;

void set_run_speed_test()
{
	Timer_ms t;
	std::vector<std::string> vs;
	constexpr int N = 1e6;
	for (int i = 0; i < N; i++)
		vs.push_back(std::to_string(i));

	Set<c_size> s;
	t.into();
	for (int i = 0; i < N; i++)
		s.insert(i);
	print("Set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(s.contains(i));

	print("Set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		s.pop(i);
	print("Set pop time: ", t.escape(), "ms");

	std::unordered_set<c_size> u;
	t.into();
	for (int i = 0; i < N; i++)
		u.insert(i);
	print("std::unordered_set insert time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		assert(u.count(i));
	print("std::unordered_set query time: ", t.escape(), "ms");
	t.into();
	for (int i = 0; i < N; i++)
		u.erase(i);
	print("std::unordered_set pop time: ", t.escape(), "ms");
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

int main()
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
