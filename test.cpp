#include "aurshine/law/implemented.hpp"
#include "aurshine/law/Array.hpp"
#include "aurshine/law/String.hpp"
#include "aurshine/law/Dynarray.hpp"

int main()
{
	using namespace ayr;

	Dynarray<int> a { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	
	print(Dynarray<int>(a));
}