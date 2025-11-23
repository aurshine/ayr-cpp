#include <ayr/net/http.hpp>
#include <ayr/net/http/Uri.hpp>

using namespace ayr;

coro::Task<void> task(coro::IoContext* io_context)
{
	auto u = net::uri("http://www.baidu.com"as);

	net::HttpResponse res = co_await net::get(io_context, u);

	print(res);
}

int main()
{
	coro::IoContext io_context;
	io_context.run(task(&io_context));
	return 0;
}