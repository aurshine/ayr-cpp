#include "ayr/filesystem.hpp"

using namespace ayr;

int main()
{
	CString file_name = fs::join(fs::dirname(__FILE__), "test.txt");
	tlog(fs::exists(file_name));
	fs::AyrFile a(file_name, "w");
	a.close();
	tlog(fs::exists(file_name));
	tlog(fs::isfile(file_name));
	tlog(fs::isdir(file_name));
	fs::remove(file_name);
	tlog(fs::exists(file_name));

	CString dir_name = fs::join(fs::dirname(__FILE__), "AAAAA_test_dir");
	tlog(fs::exists(dir_name));
	fs::mkdir(dir_name);
	tlog(fs::exists(dir_name));
	tlog(fs::isfile(dir_name));
	tlog(fs::isdir(dir_name));
	fs::remove(dir_name);
	tlog(fs::exists(dir_name));

	tlog(fs::join("home", "file.txt"));
	tlog(fs::join("/home", "/file.txt"));
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
	tlog(fs::getcwd());

	print("\n[");
	for (auto& p : fs::listdir(fs::getcwd()))
		print(p);
	print("]");

	print("\n{");
	for (auto& [root, dirs, files] : fs::walk(fs::getcwd()))
	{
		tlog(root);
		tlog(dirs);
		tlog(files);
		print("\n");
	}
	print("}");
}