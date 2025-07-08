#include <list>
#include <forward_list>

#include "ayr/Chain.hpp"
#include "ayr/timer.hpp"

using namespace ayr;


int main()
{
	Timer_ms timer;
	Chain<CString> bichain;
	std::list<CString> stdlist;
	std::forward_list<CString> stdflist;

	int N = 1e6;

	timer.into();
	for (int i = 0; i < N; i++)
		bichain.append("hello");
	print("bichain append time: ", timer.escape(), "ms");

	timer.into();
	for (int i = 0; i < N; i++)
		stdlist.push_back("hello");
	print("std::list append time: ", timer.escape(), "ms");

	timer.into();
	for (int i = 0; i < N; i++)
		stdflist.push_front("hello");
	print("std::forward_list append time: ", timer.escape(), "ms");

	timer.into();
	auto bcnd = bichain.at_node(1000);
	while (bichain.size() > 1005)
		bichain.pop(bcnd->next());
	print("bichain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto it = stdlist.begin();
	std::advance(it, 1000);
	while (stdlist.size() > 1005)
	{
		stdlist.erase(std::next(it, 1), std::next(it, 2));
	}

	print("std::list random pop time: ", timer.escape(), "ms");
	return 0;
}