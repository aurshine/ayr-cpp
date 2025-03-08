#include <ayr/net.hpp>
#include <boost/format.hpp>

using namespace ayr;

int main()
{
	boost::format fmt("Hello, %s!");
	std::cout << (fmt % "world").str();

	return 0;
}