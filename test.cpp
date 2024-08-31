#include <law/Chain.hpp>
#include <law/Dict.hpp>
#include <law/timer.hpp>
#include <law/Dynarray.hpp>
#include <law/Iterator.hpp>
#include <law/String.hpp>
#include <law/timer.hpp>
#include <law/log.hpp>

using namespace ayr;


int main()
{
	Array<int> arr = { 1, 2, 3, 4, 5 };
	Array<int> arr2 = { 1, 2, 3, 4 ,5 };
	print(arr == arr2);
}