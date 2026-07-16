#include <ayr/net/http.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	// 测试完整 URI 的 scheme、host、port、path、query 和 fragment 解析。
	net::Uri full = net::uri("https://example.com:8443/api/v1?q=openai&page=2#top"as);
	AYR_TEST_EXPECT_EQ(full.scheme(), "https"as);
	AYR_TEST_EXPECT_EQ(full.host(), "example.com"as);
	AYR_TEST_EXPECT_EQ(full.port(), "8443"as);
	AYR_TEST_EXPECT_EQ(full.path(), "/api/v1"as);
	AYR_TEST_EXPECT_EQ(full.queries().get("q"as), "openai"as);
	AYR_TEST_EXPECT_EQ(full.queries().get("page"as), "2"as);
	AYR_TEST_EXPECT_EQ(full.fragment(), "top"as);

	// 测试缺省 path 时会补为 "/"。
	net::Uri root = net::uri("http://example.com"as);
	AYR_TEST_EXPECT_EQ(root.scheme(), "http"as);
	AYR_TEST_EXPECT_EQ(root.host(), "example.com"as);
	AYR_TEST_EXPECT_EQ(root.path(), "/"as);

	// 测试只有 path/query/fragment 的相对 URI。
	net::Uri relative = net::uri("/search?q=cpp#result"as);
	AYR_TEST_EXPECT_EQ(relative.host(), ""as);
	AYR_TEST_EXPECT_EQ(relative.path(), "/search"as);
	AYR_TEST_EXPECT_EQ(relative.queries().get("q"as), "cpp"as);
	AYR_TEST_EXPECT_EQ(relative.fragment(), "result"as);

	// 测试 add_query 和 query 字符串生成。
	net::Uri manual;
	manual.scheme("https"as);
	manual.host("example.org"as);
	manual.path("/items"as);
	manual.add_query("a"as, "1"as);
	manual.add_query("b"as, "2"as);
	AYR_TEST_EXPECT(manual.query().contains("a=1"as));
	AYR_TEST_EXPECT(manual.query().contains("b=2"as));

	// 测试非法 query 字符串会抛出 AyrError。
	AYR_TEST_EXPECT_AYR_ERROR(net::uri("https://example.com/path?broken#frag"as));
}
