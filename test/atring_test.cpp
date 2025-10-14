#include <ayr/base/codec/Atring.hpp>
#include <ayr/Dict.hpp>

using namespace ayr;

int main()
{
	constexpr Atring str1 = Atring::from_utf8("你好世界");
	tlog(str1);
	constexpr Atring str2 = "123"as;
	tlog(str2);
	tlog(str1 + str2 + "456"as);
	constexpr Atring str3 = AChar('w');
	tlog(str3);
	tlog(str3 * 3);
	tlog(str3 + AChar('w'));
	Atring str4 = "你好世界我很好"as;
	str4 += "你好世界"as;
	tlog(str4);
	tlog(str4[0]);
	tlog(str4[-1]);
	tlog(str4.contains(str1));
	tlog(str4.index(str1));
	tlog(str4.rindex(str1));
	tlog(str4.count(str1));
	tlog(str4.vslice(1, 2));
	tlog(str4.vslice(5));
	tlog(str4.startswith("你好"as));
	tlog(str4.endswith("你好"as));
	tlog(str4.strip("你好"as));
	tlog(str4.strip("世界"as));
	tlog(str3.lstrip("w"as));
	tlog(str3.rstrip("w"as));
	tlog(str4.replace("你好"as, "你们好"as));
	tlog(str4.replace("你好"as, "你们好"as, 1)); //
	Atring str5 = ","as.join(arr("我"as, "爱"as, "你"as));
	tlog(str5);
	tlog(str5.split(","as));
	tlog(" 你 好 世 界 "as.split());
	tlog("114514"as.to_int());
	tlog("-114514"as.to_int());
	tlog("114514.123456"as.to_double());
	tlog("-114514.123456"as.to_double());
}

/*#include <ayr/Atring.hpp>

using namespace ayr;

int main()
{
	Atring str1 = "你好 世界";
	tlog("1"as * 5);
	tlog(str1);
	tlog(str1.slice(1, 2));
	tlog(str1.replace("你"as, "你们"as));
	Atring str2 = "aaaaa";
	tlog(str2.replace("a"as, "b"as, 2));
	tlog(str1.split());
	Atring str3 = "a1a2a3a4a5a";
	tlog(str3.split("a"as, 2));
	tlog(str1.strip("你"));
	tlog(str1.startswith("你"));
	tlog(str1.endswith("你"));
	tlog("*"as.join(arr({ "我", "爱", "你" })));
	tlog("adhaf{{这是内容}}ahfiaf{{dadf}}"as.match("{{", "}}"));
	tlog(*str1.encoding());
	tlog("123"as.to_int());
	tlog("-123"as.to_int());
	tlog("123.456"as.to_double());
	tlog("-123.456"as.to_double());
	print(UTF8->ord("你"));
}*/