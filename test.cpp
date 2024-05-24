#include "aurshine/json/json.hpp"



int main()
{
	ayr::Json j(1ll);
	std::cout << j.transform<ayr::JsonInt>();
	return 0;
}