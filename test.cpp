#include "law/implemented.hpp"
#include "law/Array.hpp"
#include "law/String.hpp"

#include <vector>

#include "law/Dynarray.hpp"
#include "law/timer.hpp"

using namespace ayr;
int N = 1e2;
DynArray<std::string> da;
std::vector<std::string> v;

void t1()
{
	for (int i = 0; i < N; ++i)
		da.append("hello world");
}

void t2()
{
	for (int i = 0; i < N; ++i)
		v.push_back("hello world");
}

int main()
{
	Astring as;
	as = "aba";

	print(as.split("aba"));
}