#include "aurshine/law/implemented.hpp"
#include "aurshine/law/Array.hpp"

int main()
{
	ayr::Array<int> a{ 1, 2, 3, 5};
	ayr::print(a.slice(1, -35));
	return 0;
}