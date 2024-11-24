#include "buffer_test.hpp"
#include "dynarray_test.hpp"
#include "socket_test.hpp"
#include "atring_test.hpp"
#include "json_test.hpp"
#include "dict_test.hpp"

using namespace ayr;

int main()
{
	dict_test();
	set_test();
	return 0;
}