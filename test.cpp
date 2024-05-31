#include <vector>

#include <thread/Lock.hpp>
#include <law/Dynarray.hpp>
#include <law/timer.hpp>
#include <law/Array.hpp>
#include <law/AString.hpp>
#include <law/CString.hpp>

#include <unordered_map>
using namespace ayr;

int main()
{
	std::unordered_map<CString, int> map;
	map["hello"] = 1;
	map["world"] = 2;
	map["world"] = 3;
	map["world"] = 4;
	print(map["world"]);
}