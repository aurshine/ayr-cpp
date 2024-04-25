#include "law/law.hpp"
#include "json/parse.hpp"
#include "thread/threadpool.hpp"

void dfs(int id, int bg)
{
	ayr::print("id =", id, "value =", bg);
}

int main()
{
	using namespace ayr;
	ThreadPool pool(5);
	for (int i = 0; i < 5000; ++i)
		pool.push(dfs, i, i + 5000);


	return 0;
}