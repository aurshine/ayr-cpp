#include <array>

#include "ayr/base/Array.hpp"
#include "ayr/base/printer.hpp"
#include "ayr/timer.hpp"


using namespace ayr;

void print_info(const CString& s, const CString& name)
{
	print(std::format("owner: {}, sso: {}, size: {}, {}: {}", s.owner(), s.sso(), s.size(), name, s));
}

void cstr_test()
{
	Timer_ms timer;
	constexpr int N = 1e6;
	timer.into();
	for (double i = 0; i < N; ++i)
		cstr(i);
	auto t1 = timer.escape();

	timer.into();
	for (double i = 0; i < N; ++i)
		std::to_string(i);
	auto t2 = timer.escape();

	print("cstr:", t1, "ms");
	print("std::to_string:", t2, "ms");
}

int main()
{
	auto s1 = vstr("Hello, world!");
	auto s2 = s1.clone();
	auto s3 = dstr("你好，世界！");
	char* p = ayr_alloc<char>(19);
	std::memcpy(p, "你好，世界！", 19);
	auto s4 = ostr(p, 19);

	print_info(s1, "s1");
	print_info(s2, "s2");
	print_info(s3, "s3");
	print_info(s4, "s4");
	print_info(s1 + s3, "s5");
	print_info(vstr("hello") + vstr("world"), "s6");
	print_info(CString::cjoin(arr(vstr("h"), vstr("e"), vstr("l"), vstr("l"), vstr("o"))), "s7");
	print_info(vstr("-").join(arr(vstr("h"), vstr("e"), vstr("l"), vstr("l"), vstr("o"))), "s8");

	print("s1 == s2: ", s1 == s2);
	print("s1 == s3: ", s1 == s3);
	print("s3 == s4: ", s3 == s4);

	std::swap(s1, s3);
	print_info(s1, "s1 after swap");
	print_info(s3, "s3 after swap");
	print_info(s1.slice(0, 6), "s1.slice(0, 6)");
	print_info(s3.slice(0, 5), "s3.slice(0, 5)");
	print_info(s1.vslice(0, 6), "s1.vslice(0, 6)");
	print_info(s3.vslice(0, 5), "s3.vslice(0, 5)");
	print_info(s1.slice(9), "s1.slice(9)");
	print_info(s3.slice(7), "s3.slice(7)");
	print_info(s1.vslice(9), "s1.vslice(9)");
	print_info(s3.vslice(7), "s3.vslice(7)");

	tlog(cstr(1));
	tlog(cstr('a'));
	tlog(cstr(1.0));
	tlog(cstr(true));
	tlog(cstr(nullptr));
	tlog(cstr(std::pair(1, 2)));
	tlog(cstr(std::tuple(1, 2, 3)));
	tlog(cstr(std::array<int, 3>{1, 2, 3}));

	cstr_test();
	return 0;
}