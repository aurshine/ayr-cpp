#include "aurshine/json/json.hpp"
#include <typeinfo>


int main()
{
	ayr::json::Json j(ayr::json::make_int(5ll));
	std::cout << j.transform<ayr::json::JsonType::JsonInt>() << " " << j.type();
	return 0;
}