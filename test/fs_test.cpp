#include "ayr/filesystem.hpp"

using namespace ayr;

int main()
{
	tlog(fs::basename("/home/user/file.txt"));
	tlog(fs::basename("a.txt"));
	tlog(fs::basename("/"));
	tlog(fs::dirname("/home/user/file.txt"));
	tlog(fs::dirname("a.txt"));
	tlog(fs::dirname("/"));
	tlog(fs::splitext("/home/user/file.txt"));
	tlog(fs::splitext("a.txt"));
	tlog(fs::splitext("abc"));
	tlog(fs::split("/home/user/file.txt"));
	tlog(fs::split("a.txt"));
	tlog(fs::split("abc"));
}