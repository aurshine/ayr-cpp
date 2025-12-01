#include <array>

#include <ayr/base.hpp>

using namespace ayr;

int main()
{
	constexpr Atring str1 = Atring::from_utf8("你好世界");
	tlog(str1);
	constexpr Atring str2 = "1"as + "2"as + "3"as;
	tlog(str2);
	constexpr Atring str3 = AChar('w');
	tlog(str3);
	constexpr Atring str4 = str3 * 3;
	tlog(str4);
	Atring str5 = "你好世界我很好"as;
	str5 += "你好世界"as;
	tlog(str5);
	constexpr AChar str2_0 = str2[0];
	tlog(str2_0);
	constexpr AChar str2__1 = str2[-1];
	tlog(str2__1);
	constexpr bool contains = str1.contains("你好"as);
	tlog(contains);
	constexpr c_size idx = str4.index("w"as);
	tlog(idx);
	constexpr c_size ridx = str4.rindex("w"as);
	tlog(ridx);
	constexpr c_size cnt = str4.count("w"as);
	tlog(cnt);
	constexpr Atring str6 = str1.vslice(0, 2);
	tlog(str6);
	constexpr Atring str7 = str1.vslice(2);
	tlog(str7);
	constexpr bool sw1 = str1.startswith("你好"as);
	tlog(sw1);
	constexpr bool sw2 = str1.startswith("世界"as);
	tlog(sw2);
	constexpr bool ew1 = str1.endswith("世界"as);
	tlog(ew1);
	constexpr bool ew2 = str1.endswith("你好"as);
	tlog(ew2);
	constexpr Atring str8 = str1.strip("你好"as);
	tlog(str8);
	constexpr Atring str9 = str1.lstrip("你好"as);
	tlog(str9);
	constexpr Atring str10 = str1.rstrip("世界"as);
	tlog(str10);
	constexpr Atring str11 = str1.replace("你好"as, "你们好"as);
	tlog(str11);
	constexpr Atring str12 = ","as.join(std::array<Atring, 3>{ "我"as, "爱"as, "你"as });
	tlog(str12);
	tlog(str12.split(","as));
	tlog(" 你 好 世 界 "as.split());
	constexpr c_size num1 = "114514"as.toint().first;
	tlog(num1);
	constexpr c_size num2 = "-11451"as.toint().first;
	tlog(num2);
	constexpr double num3 = "114.51"as.tofloat().first;
	tlog(num3);
	constexpr double num4 = "-114.5"as.tofloat().first;
	tlog(num4);
}