#pragma once
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

	tlog(bichain.size());
	for (auto& i : bichain)
		tlog(i);

	auto it = bichain.at_node(bichain.size() - 1);
	while (it)
	{
		tlog(*it);
		it = it->prev_node();
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
	int N = 1e6;

	timer.into();
	for (int i = 0; i < N; i++)
		chain.append("hello");
	tlog(timer.escape());

	timer.into();
	for (int i = 0; i < N; i++)
		bichain.append("hello");
	tlog(timer.escape());

	timer.into();
	while (chain.size())
		chain.pop_from(chain.at_node(0), 1);
	tlog(timer.escape());

	timer.into();
	while (bichain.size())
		bichain.pop_from(bichain.at_node(0), 1);
	tlog(timer.escape());
}