#include "law/implemented.hpp"
#include "law/Array.hpp"
#include "law/String.hpp"
#include "law/Dynarray.hpp"

int main()
{
	using namespace ayr;

	Dynarray<int> a{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	print(Dynarray<int>(a));
}