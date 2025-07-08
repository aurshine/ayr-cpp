#include <ayr/Atring.hpp>

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
}