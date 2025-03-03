#include <ayr/Atring.hpp>

using namespace ayr;

void atring_test()
{
	Atring str1 = "你好 世界";
	tlog(str1);
	tlog(str1.slice(1, 2));
	tlog(str1.replace("你"as, "你们"as));
	tlog(str1.split());
	tlog(str1.strip("你"));
	tlog(str1.startswith("你"));
	tlog(str1.endswith("你"));
	tlog("*"as.join(Array<const char*>{ "我", "爱", "你" }));
	tlog("adhaf{{这是内容}}ahfiaf{{dadf}}"as.match("{{", "}}"));
	tlog(*str1.encoding());
}