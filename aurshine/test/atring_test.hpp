#include <ayr/Atring.hpp>

using namespace ayr;

void atring_test()
{
	Atring str1 = "你好世界";
	print(str1);
	Atring str2 = str1;
	Atring str3 = str2.slice(0);

	str3[1] = 'a';
	print(str1, str2, str3);

	print(str1.replace("你"as, "你们"as));
	print(str1.slice(0, str1.size()));

	Atring str4 = "{a, b, c, d, [1, 2, 3], {x: 1, y: 2}}";

	print(str4.match('{', '}'));
}