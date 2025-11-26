#include <ayr/net/http.hpp>
#include <ayr/net/http/Uri.hpp>

using namespace ayr;

coro::Task<void> task(coro::IoContext* io_context)
{
	auto u = net::uri("https://www.acwing.com"as);
	net::HttpResponse res = co_await get(io_context, u);

	print(res);
}

int main()
{
	coro::IoContext io_context;
	io_context.run(task(&io_context));

}