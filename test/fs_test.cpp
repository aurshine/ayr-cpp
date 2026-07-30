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

	UTEST_SCOPE("测试目录创建、存在性、文件/目录类型判断和 exist_ok。")
	{
		UTEST_EXPECT(!fs::exists(root));
		fs::mkdir(root);
		fs::mkdir(root, true);
		UTEST_EXPECT_AYR_ERROR(fs::mkdir(root));
		UTEST_EXPECT(fs::exists(root));
		UTEST_EXPECT(fs::isdir(root));
		UTEST_EXPECT(!fs::isfile(root));
		UTEST_EXPECT(!fs::isdir(fs::join(root, "missing")));
		UTEST_EXPECT_AYR_ERROR(consume_directory(fs::join(root, "missing")));
	};

	CString file_path = fs::join(root, "sample.txt");
	UTEST_SCOPE("测试文件创建、写入、读取和文件大小。")
	{
		{
			fs::AyrFile file(file_path, "w");
			file.write("hello");
		}
		UTEST_EXPECT(fs::exists(file_path));
		UTEST_EXPECT(fs::isfile(file_path));
		UTEST_EXPECT_EQ(fs::filesize(file_path), 5);
		{
			fs::AyrFile file(file_path, "r");
			UTEST_EXPECT_EQ(file.read(0), "");
			UTEST_EXPECT_EQ(file.read(2), "he");
			UTEST_EXPECT_EQ(file.read(), "llo");
		}
		UTEST_EXPR(fs::AyrFile(file_path, "r").read(-2));
		UTEST_EXPR(fs::AyrFile(file_path, "w").write("x"));
		fs::write(file_path, "hello");
	};

	UTEST_SCOPE("测试 exist_ok 不能掩盖同名普通文件。")
	{
		UTEST_EXPECT_AYR_ERROR(fs::mkdir(file_path, true));
	};

	UTEST_SCOPE("测试路径拼接和拆分逻辑。")
	{
		CString nested = fs::join(root, "nested");
		fs::mkdir(nested);
		CString nested_file = fs::join(nested, "data.bin");
		{
			fs::AyrFile file(nested_file, "w");
			file.write("abc");
		}
		UTEST_EXPECT(fs::join("home", "file.txt").contains("home"));
		UTEST_EXPECT_EQ(fs::join("", "file.txt"), "file.txt");
		UTEST_EXPECT_EQ(fs::join("home", ""), "home");
		UTEST_EXPECT_EQ(fs::basename("/home/user/file.txt"), "file.txt");
		UTEST_EXPECT_EQ(fs::dirname("/home/user/file.txt"), "/home/user");
		auto [base, ext] = fs::splitext("/home/user/file.txt");
		UTEST_EXPECT_EQ(base, "/home/user/file");
		UTEST_EXPECT_EQ(ext, ".txt");
		auto [dot_dir_base, dot_dir_ext] = fs::splitext("/home.with.dot/file");
		UTEST_EXPECT_EQ(dot_dir_base, "/home.with.dot/file");
		UTEST_EXPECT_EQ(dot_dir_ext, "");
		auto [dot_file_base, dot_file_ext] = fs::splitext("/home/user/.profile");
		UTEST_EXPECT_EQ(dot_file_base, "/home/user/.profile");
		UTEST_EXPECT_EQ(dot_file_ext, "");
		auto [dir, name] = fs::split("/home/user/file.txt");
		UTEST_EXPECT_EQ(dir, "/home/user");
		UTEST_EXPECT_EQ(name, "file.txt");
#ifdef AYR_WIN
		UTEST_EXPECT(!fs::isabs("C:relative.txt"));
		UTEST_EXPECT(fs::isabs("C:\\absolute.txt"));
#endif
	};

	UTEST_SCOPE("测试 CRLF/LF 均能被识别，并且结果不包含换行字符。")
	{
		CString lines_path = fs::join(root, "lines.txt");
		fs::write(lines_path, "first\r\n\r\nlast");
		c_size line_index = 0;
		for (auto& line : fs::AyrFile(lines_path, "r").readlines())
		{
			if (line_index == 0)
				UTEST_EXPECT_EQ(line, "first");
			else if (line_index == 1)
				UTEST_EXPECT_EQ(line, "");
			else if (line_index == 2)
				UTEST_EXPECT_EQ(line, "last");
			++line_index;
		}
		UTEST_EXPECT_EQ(line_index, 3);

		DynArray<CString> output_lines;
		output_lines.append(cstr("one"));
		output_lines.append(cstr("two"));
		CString written_lines_path = fs::join(root, "written_lines.txt");
		fs::writelines(written_lines_path, output_lines);
#ifdef AYR_WIN
		UTEST_EXPECT_EQ(fs::read(written_lines_path), "one\r\ntwo\r\n");
#else
		UTEST_EXPECT_EQ(fs::read(written_lines_path), "one\ntwo\n");
#endif
	};

	UTEST_SCOPE("测试 listdir 能看到创建的条目。")
	{
		bool saw_file = false;
		bool saw_dir = false;
		for (auto& entry : fs::listdir(root))
		{
			if (entry == "sample.txt")
				saw_file = true;
			if (entry == "nested")
				saw_dir = true;
		}
		UTEST_EXPECT(saw_file);
		UTEST_EXPECT(saw_dir);
	};

	UTEST_SCOPE("测试 walk 至少返回根目录，并检查目录大小和异常路径。")
	{
		bool saw_root = false;
		for (auto& [walk_root, dirs, files] : fs::walk(root))
		{
			(void)dirs;
			(void)files;
			if (walk_root == root)
				saw_root = true;
		}
		UTEST_EXPECT(saw_root);
#ifdef AYR_WIN
		UTEST_EXPECT_EQ(fs::filesize(root), 31);
#else
		UTEST_EXPECT_EQ(fs::filesize(root), 29);
#endif
		UTEST_EXPECT_AYR_ERROR(fs::filesize(fs::join(root, "missing")));
	};

#if defined(AYR_LINUX) || defined(AYR_MAC)
	UTEST_SCOPE("测试删除目录链接只删除链接本身，不删除目标目录内容。")
	{
		CString link_target = fs::join(root, "link_target");
		fs::mkdir(link_target);
		CString protected_file = fs::join(link_target, "keep.txt");
		fs::write(protected_file, "keep");
		CString directory_link = fs::join(root, "directory_link");
		if (::symlink(link_target.c_str(), directory_link.c_str()) != 0)
			SystemError(ayr::format("Failed to create test symlink, {}", get_system_error_msg()));
		fs::remove(directory_link);
		UTEST_EXPECT(fs::exists(protected_file));
		UTEST_EXPECT(!fs::exists(directory_link));
	};
#endif

	UTEST_SCOPE("测试递归删除能清理非空目录。")
	{
		fs::remove(root);
		UTEST_EXPECT(!fs::exists(root));
	};

	return UTEST_COMPLETE();
}
