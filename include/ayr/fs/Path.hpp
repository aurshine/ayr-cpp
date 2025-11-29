#ifndef AYR_FS_PATH_HPP
#define AYR_FS_PATH_HPP

#include <cstdlib>
#include <queue>

#include "oslib.h"
#include "../air/DynArray.hpp"
#include "../coro/Generator.hpp"

namespace ayr
{
	namespace fs
	{
#ifdef AYR_WIN
		constexpr char PATH_SEP = '\\';
#else
		constexpr char PATH_SEP = '/';
#endif

		bool is_sep(char c) { return c == '\\' || c == '/'; }

		void raise_error_msg_for_path(const CString& path) { SystemError(std::format("{}: {}", get_error_msg(), path)); }

		// 拼接两个路径
		def join(const CString& path1, const CString& path2)
		{
			c_size len1 = path1.size();
			c_size len2 = path2.size();
			while (is_sep(path1[len1 - 1]))
				--len1;

			c_size i = 0;
			while (i < len2 && is_sep(path2[i]))
				++i;

			return CString::cjoin(arr(path1.vslice(0, len1), cstr(PATH_SEP), path2.vslice(i)));
		}

		// 是否存在路径
		def exists(const CString& path)
		{
#ifdef AYR_WIN
			return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
			return access(path.c_str(), F_OK) == 0;
#endif
		}

		// 是否是文件
		def isfile(const CString& path)
		{
#ifdef AYR_WIN
			DWORD attributes = GetFileAttributesA(path.c_str());

			return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
			struct stat st;
			if (stat(path.c_str(), &st) == 0)
				return S_ISREG(st.st_mode);
			return false;
#endif
		}

		// 是否是文件夹
		def isdir(const CString& path)
		{
#ifdef AYR_WIN
			return (GetFileAttributesA(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
			struct stat st;
			if (stat(path.c_str(), &st) == 0)
				return S_ISDIR(st.st_mode);
			return false;
#endif
		}

		// 是否是绝对路径
		def isabs(const CString& path)
		{
			if (path.empty()) return false;
#ifdef AYR_WIN
			// Windows 绝对路径情况：例如 "C:\path" 或 "\\Server\share"
			return path.size() >= 2 && (
				(std::isalpha(path[0]) && path[1] == ':') ||
				(is_sep(path[0]) && is_sep(path[1])
					));
#else
			// Unix/Linux/macOS：以 '/' 开头即为绝对路径
			return is_sep(path[0]);
#endif
		}

		// 获得绝对路径
		def abspath(const CString& path)
		{
#ifdef AYR_WIN
			char abs_path[MAX_PATH];
			DWORD len = GetFullPathNameA(path.c_str(), MAX_PATH, abs_path, nullptr);

			if (len == 0 || len > MAX_PATH)
				SystemError(std::format("abspath failed for path: {}", path));
			return dstr(abs_path, len);
#else
			char* abs_path = realpath(path.c_str(), nullptr);

			if (abs_path == nullptr)
				SystemError(std::format("abspath failed for path: {}", path));
			return ostr(abs_path);
#endif 
		}

		// 获得当前路径
		def getcwd()
		{
#ifdef AYR_WIN
			char cwd_path[MAX_PATH];
			DWORD len = GetCurrentDirectoryA(MAX_PATH, cwd_path);

			if (len == 0 || len > MAX_PATH)
				SystemError("getcwd failed");
			return dstr(cwd_path, len);
#else
			char cwd_path[PATH_MAX];
			if (::getcwd(cwd_path, PATH_MAX) == nullptr)
				SystemError(get_error_msg());
			return dstr(cwd_path);
#endif
		}

		// 列出当前路径下的所有文件和文件夹
		def listdir(const CString& path) -> coro::Generator<CString>
		{
#ifdef AYR_WIN
			WIN32_FIND_DATAA find_data;

			HANDLE handle = FindFirstFileA(join(path, "*").c_str(), &find_data);

			ExTask task([&handle] { FindClose(handle); });

			if (handle == INVALID_HANDLE_VALUE)
				raise_error_msg_for_path(path);

			do
			{
				CString file_name = dstr(find_data.cFileName);
				if (file_name == "." || file_name == "..")
					continue;

				co_yield std::move(file_name);
			} while (FindNextFileA(handle, &find_data));
#else
			DIR* dir = opendir(path.c_str());
			ExTask task([&dir] { closedir(dir); });

			if (dir == nullptr)
				raise_error_msg_for_path(path);

			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr)
			{
				CString d_name = dstr(entry->d_name);
				if (d_name == "." || d_name == "..")
					continue;
				co_yield std::move(d_name);
			}
#endif
		}

		// 遍历当前路径下的所有文件和文件夹
		def walk(const CString& path) -> coro::Generator<std::tuple<CString, Array<CString>, Array<CString>>>
		{
			DynArray<std::tuple<CString, Array<CString>, Array<CString>>> results;
			std::queue<CString> root_dirs;
			root_dirs.push(path.clone());

			while (!root_dirs.empty())
			{
				CString root = std::move(root_dirs.front());
				root_dirs.pop();

				DynArray<CString> dirs, files;
				for (auto& sub_path : listdir(root))
					if (isfile(join(root, sub_path)))
						files.append(sub_path.clone());
					else
						dirs.append(sub_path.clone());

				co_yield std::make_tuple(std::move(root), dirs.move_array(), files.move_array());
			}
		}

		// 用当前路径创建一个文件夹
		def mkdir(const CString& path, bool exist_ok = false)
		{
#ifdef AYR_WIN
			BOOL state = CreateDirectoryA(path.c_str(), nullptr);

			switch (state)
			{
			case ERROR_ALREADY_EXISTS:
				if (!exist_ok)
					raise_error_msg_for_path(path);
				break;
			case ERROR_PATH_NOT_FOUND:
				raise_error_msg_for_path(path);
				break;
			}
#else
			if (::mkdir(path.c_str(), 0755) != 0)
				raise_error_msg_for_path(path);
#endif
		}

		// 删除当前路径的文件或文件夹
		def remove(const CString& path) -> void
		{
			if (!exists(path)) return;
			if (isfile(path))
			{
#ifdef AYR_WIN
				DeleteFileA(path.c_str());
#else
				if (::unlink(path.c_str()) != 0)
					raise_error_msg_for_path(path);
#endif
			}
			else
			{
				for (auto& sub_path : listdir(path))
					remove(join(path, sub_path));

#ifdef AYR_WIN
				RemoveDirectoryA(path.c_str());
#else
				if (::rmdir(path.c_str()) != 0)
					raise_error_msg_for_path(path);
#endif
			}
		}

		// 获得当前路径的文件或文件夹大小，单位为字节
		def filesize(const CString& path) -> uint64_t
		{
			uint64_t size_ = 0;
			if (isfile(path))
			{
#ifdef AYR_WIN
				WIN32_FILE_ATTRIBUTE_DATA attr;
				if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &attr))
					raise_error_msg_for_path(path);
				size_ = (static_cast<uint64_t>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
#else
				struct stat st;
				if (stat(path.c_str(), &st) == 0)
					size_ = st.st_size;
				else
					raise_error_msg_for_path(path);
#endif 
			}
			else
			{
				for (auto&& sub_path : listdir(path))
					size_ += filesize(join(path, sub_path));
			}
			return size_;
		}

		// 获得路径的基本名
		def basename(const CString& path)
		{
			c_size last_sep_pos = -1;
			for (c_size i = path.size() - 1; i >= 0; --i)
				if (is_sep(path[i]))
				{
					last_sep_pos = i;
					break;
				}

			return path.slice(last_sep_pos + 1);
		}

		// 获得路径的目录
		def dirname(const CString& path) -> CString
		{
			c_size last_sep_pos = -1;
			for (c_size i = path.size() - 1; i >= 0; --i)
				if (is_sep(path[i]))
				{
					last_sep_pos = i;
					break;
				}

			if (last_sep_pos == -1)
				return "";
			else if (last_sep_pos == 0)
				return cstr(PATH_SEP);

			return path.slice(0, last_sep_pos);
		}

		// 获得路径的目录和文件名
		def split(const CString& path)
		{
			return std::make_pair(dirname(path), basename(path));
		}

		// 获得文件扩展名
		def splitext(const CString& path)
		{
			c_size last_dot_pos = -1, pos = 0;
			for (c_size i = path.size() - 1; i >= 0; --i)
				if (path[i] == '.')
				{
					last_dot_pos = i;
					break;
				}

			if (last_dot_pos == -1) last_dot_pos = path.size();

			return std::make_pair(
				path.slice(0, last_dot_pos),
				path.slice(last_dot_pos)
			);
		}
	}
}
#endif // AYR_FS_PATH_HPP