#include <ayr/filesystem.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

void consume_directory(const CString& path)
{
	for (auto& entry : fs::listdir(path))
		(void)entry;
}

int main()
{
	CString root = fs::join(fs::dirname(__FILE__), "tmp_fs_test");
	fs::remove(root);

	// 测试目录创建、存在性、文件/目录类型判断和 exist_ok。
	AYR_TEST_EXPECT(!fs::exists(root));
	fs::mkdir(root);
	fs::mkdir(root, true);
	AYR_TEST_EXPECT_AYR_ERROR(fs::mkdir(root));
	AYR_TEST_EXPECT(fs::exists(root));
	AYR_TEST_EXPECT(fs::isdir(root));
	AYR_TEST_EXPECT(!fs::isfile(root));
	AYR_TEST_EXPECT(!fs::isdir(fs::join(root, "missing")));
	AYR_TEST_EXPECT_AYR_ERROR(consume_directory(fs::join(root, "missing")));

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
		AYR_TEST_EXPECT_EQ(file.read(0), "");
		AYR_TEST_EXPECT_EQ(file.read(2), "he");
		AYR_TEST_EXPECT_EQ(file.read(), "llo");
	}
	AYR_TEST_EXPECT_RUN(fs::AyrFile(file_path, "r").read(-2));
	AYR_TEST_EXPECT_RUN(fs::AyrFile(file_path, "w").write("x"));
	fs::write(file_path, "hello");

	// exist_ok 不能掩盖同名普通文件。
	AYR_TEST_EXPECT_AYR_ERROR(fs::mkdir(file_path, true));

	// 测试路径拼接和拆分逻辑。
	CString nested = fs::join(root, "nested");
	fs::mkdir(nested);
	CString nested_file = fs::join(nested, "data.bin");
	{
		fs::AyrFile file(nested_file, "w");
		file.write("abc");
	}
	AYR_TEST_EXPECT(fs::join("home", "file.txt").contains("home"));
	AYR_TEST_EXPECT_EQ(fs::join("", "file.txt"), "file.txt");
	AYR_TEST_EXPECT_EQ(fs::join("home", ""), "home");
	AYR_TEST_EXPECT_EQ(fs::basename("/home/user/file.txt"), "file.txt");
	AYR_TEST_EXPECT_EQ(fs::dirname("/home/user/file.txt"), "/home/user");
	auto [base, ext] = fs::splitext("/home/user/file.txt");
	AYR_TEST_EXPECT_EQ(base, "/home/user/file");
	AYR_TEST_EXPECT_EQ(ext, ".txt");
	auto [dot_dir_base, dot_dir_ext] = fs::splitext("/home.with.dot/file");
	AYR_TEST_EXPECT_EQ(dot_dir_base, "/home.with.dot/file");
	AYR_TEST_EXPECT_EQ(dot_dir_ext, "");
	auto [dot_file_base, dot_file_ext] = fs::splitext("/home/user/.profile");
	AYR_TEST_EXPECT_EQ(dot_file_base, "/home/user/.profile");
	AYR_TEST_EXPECT_EQ(dot_file_ext, "");
	auto [dir, name] = fs::split("/home/user/file.txt");
	AYR_TEST_EXPECT_EQ(dir, "/home/user");
	AYR_TEST_EXPECT_EQ(name, "file.txt");
#ifdef AYR_WIN
	AYR_TEST_EXPECT(!fs::isabs("C:relative.txt"));
	AYR_TEST_EXPECT(fs::isabs("C:\\absolute.txt"));
#endif

	// CRLF/LF 都应被识别，并且结果不包含换行字符。
	CString lines_path = fs::join(root, "lines.txt");
	fs::write(lines_path, "first\r\n\r\nlast");
	c_size line_index = 0;
	for (auto& line : fs::AyrFile(lines_path, "r").readlines())
	{
		if (line_index == 0)
			AYR_TEST_EXPECT_EQ(line, "first");
		else if (line_index == 1)
			AYR_TEST_EXPECT_EQ(line, "");
		else if (line_index == 2)
			AYR_TEST_EXPECT_EQ(line, "last");
		++line_index;
	}
	AYR_TEST_EXPECT_EQ(line_index, 3);

	DynArray<CString> output_lines;
	output_lines.append(cstr("one"));
	output_lines.append(cstr("two"));
	CString written_lines_path = fs::join(root, "written_lines.txt");
	fs::writelines(written_lines_path, output_lines);
#ifdef AYR_WIN
	AYR_TEST_EXPECT_EQ(fs::read(written_lines_path), "one\r\ntwo\r\n");
#else
	AYR_TEST_EXPECT_EQ(fs::read(written_lines_path), "one\ntwo\n");
#endif

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
#ifdef AYR_WIN
	AYR_TEST_EXPECT_EQ(fs::filesize(root), 31);
#else
	AYR_TEST_EXPECT_EQ(fs::filesize(root), 29);
#endif
	AYR_TEST_EXPECT_AYR_ERROR(fs::filesize(fs::join(root, "missing")));

#if defined(AYR_LINUX) || defined(AYR_MAC)
	// 删除目录链接只能删除链接本身，不能进入并删除目标目录内容。
	CString link_target = fs::join(root, "link_target");
	fs::mkdir(link_target);
	CString protected_file = fs::join(link_target, "keep.txt");
	fs::write(protected_file, "keep");
	CString directory_link = fs::join(root, "directory_link");
	if (::symlink(link_target.c_str(), directory_link.c_str()) != 0)
		SystemError(ayr::format("Failed to create test symlink, {}", get_system_error_msg()));
	fs::remove(directory_link);
	AYR_TEST_EXPECT(fs::exists(protected_file));
	AYR_TEST_EXPECT(!fs::exists(directory_link));
#endif

	fs::remove(root);
	AYR_TEST_EXPECT(!fs::exists(root));
}
