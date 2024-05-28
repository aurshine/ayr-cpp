#include "aurshine/law/implemented.hpp"
#include "aurshine/law/Array.hpp"
#include "aurshine/law/String.hpp"

int main()
{
	ayr::AString<char> str = "asddf";
	ayr::print(ayr::AString("<<").join({"123", "abc", "efj"}));
}