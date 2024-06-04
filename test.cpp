#include <law/Chain.hpp>

using namespace ayr;

int main()
{
	BiChain<int> chain;
	chain.append(1);
	chain.append(2);
	chain.append(3);
	chain.append(4);
	print(chain.to_array());
	return 0;
}