#include <ayr/filesystem.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	CString root = fs::join(fs::dirname(__FILE__), "tmp_fs_test");
	fs::remove(root);

	// 测试目录创建、存在性、文件/目录类型判断和 exist_ok。
	AYR_TEST_EXPECT(!fs::exists(root));
	fs::mkdir(root);
	fs::mkdir(root, true);
	AYR_TEST_EXPECT(fs::exists(root));
	AYR_TEST_EXPECT(fs::isdir(root));
	AYR_TEST_EXPECT(!fs::isfile(root));

	// 测试文件创建、写入、读取和文件大小。
	CString file_path = fs::join(root, "sample.txt");
	{
		fs::AyrFile file(file_path, "w");
		file.write("hello");
	}
	AYR_TEST_EXPECT(fs::exists(file_path));
	AYR_TEST_EXPECT(fs::isfile(file_path));
	AYR_TEST_EXPECT_EQ(fs::filesize(file_path), 5);
	{
		fs::AyrFile file(file_path, "r");
		AYR_TEST_EXPECT_EQ(file.read(), "hello");
	}

	// 测试路径拼接和拆分逻辑。
	CString nested = fs::join(root, "nested");
	fs::mkdir(nested);
	CString nested_file = fs::join(nested, "data.bin");
	{
		fs::AyrFile file(nested_file, "w");
		file.write("abc");
	}
	AYR_TEST_EXPECT(fs::join("home", "file.txt").contains("home"));
	AYR_TEST_EXPECT_EQ(fs::basename("/home/user/file.txt"), "file.txt");
	AYR_TEST_EXPECT_EQ(fs::dirname("/home/user/file.txt"), "/home/user");
	auto [base, ext] = fs::splitext("/home/user/file.txt");
	AYR_TEST_EXPECT_EQ(base, "/home/user/file");
	AYR_TEST_EXPECT_EQ(ext, ".txt");
	auto [dir, name] = fs::split("/home/user/file.txt");
	AYR_TEST_EXPECT_EQ(dir, "/home/user");
	AYR_TEST_EXPECT_EQ(name, "file.txt");

	// 测试 listdir 能看到创建的条目。
	bool saw_file = false;
	bool saw_dir = false;
	for (auto& entry : fs::listdir(root))
	{
		if (entry == "sample.txt")
			saw_file = true;
		if (entry == "nested")
			saw_dir = true;
	}
	AYR_TEST_EXPECT(saw_file);
	AYR_TEST_EXPECT(saw_dir);

	// 测试 walk 至少返回根目录，并且递归删除能清理非空目录。
	bool saw_root = false;
	for (auto& [walk_root, dirs, files] : fs::walk(root))
	{
		(void)dirs;
		(void)files;
		if (walk_root == root)
			saw_root = true;
	}
	AYR_TEST_EXPECT(saw_root);
	AYR_TEST_EXPECT_EQ(fs::filesize(root), 8);
	fs::remove(root);
	AYR_TEST_EXPECT(!fs::exists(root));
}
