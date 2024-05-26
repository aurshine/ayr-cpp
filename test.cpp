#include "aurshine/json/json.hpp"
#include <typeinfo>


int main()
{
	ayr::json::Json j(std::string("hello world"));
	std::cout << j.transform<ayr::json::JsonStr>() << " " << j.type();
	return 0;
}