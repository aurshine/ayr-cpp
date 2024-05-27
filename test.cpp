#include "aurshine/law/implemented.hpp"
#include "aurshine/law/Array.hpp"
#include "aurshine/law/String.hpp"

int main()
{
	ayr::AString<char> str = "Hello, world! ";
	ayr::print(str + "This is a test.");
}