#include <list>
#include <forward_list>

#include "ayr/Chain.hpp"
#include "ayr/timer.hpp"

using namespace ayr;

inline void chain_test()
{
	Chain<int> chain;

	for (int i = 0; i < 10; i++)
		chain.append(i);

	tlog(chain.size());
	for (auto& i : chain)
		tlog(i);

	chain.pop_back(3);
	tlog(chain.size());
	for (auto& i : chain)
		tlog(i);

	chain.pop_if([](int i) { return i % 2 == 0; });
	tlog(chain.size());
	for (auto& i : chain)
		tlog(i);

	tlog(chain);
}

inline void bichain_test()
{
	BiChain<int> bichain;
	for (int i = 0; i < 10; i++)
		bichain.append(i);
	auto& node = bichain.at_node(2);

	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	auto it = bichain.at_node(bichain.size() - 1);
	while (it.has_prev())
	{
		tlog(*it);
		it = *it.prev_node();
	}

	bichain.pop_back(3);
	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	bichain.pop_if([](int i) { return i % 2 == 0; });
	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	for (int i = 0; i > -3; --i)
		bichain.prepend(i);

	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	bichain.pop_from(bichain.at_node(2), 2);
	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	tlog(bichain);
	bichain.pop_back(bichain.size());
	tlog(bichain.size());
}

def chain_speed_test()
{
	Timer_ms timer;
	Chain<CString> chain;
	BiChain<CString> bichain;
	std::list<CString> stdlist;
	std::forward_list<CString> stdflist;

	int N = 1e6;

	timer.into();
	for (int i = 0; i < N; i++)
		chain.append("hello");
	print("chain append time: ", timer.escape(), "ms");

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
	auto& cnd = chain.at_node(1000);
	while (chain.size() > 1005)
		chain.pop_from(*cnd.next_node(), 1, &cnd);
	print("chain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto& bcnd = bichain.at_node(1000);
	while (bichain.size() > 1005)
		bichain.pop_from(*bcnd.next_node(), 1);
	print("bichain random pop time: ", timer.escape(), "ms");

	timer.into();
	auto it = stdlist.begin();
	std::advance(it, 1000);
	while (stdlist.size() > 1005)
	{
		stdlist.erase(std::next(it, 1), std::next(it, 2));
	}

	print("std::list random pop time: ", timer.escape(), "ms");

}