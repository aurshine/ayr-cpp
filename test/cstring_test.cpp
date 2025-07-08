#include "ayr/base/Array.hpp"
#include "ayr/base/printer.hpp"

using namespace ayr;

void print_info(const CString& s, const CString& name)
{
	print(std::format("owner: {}, sso: {}, size: {}, {}: {}", s.owner(), s.sso(), s.size(), name, s));
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

	swap(s1, s3);
	print_info(s1, "s1 after swap");
	print_info(s3, "s3 after swap");
	return 0;
}