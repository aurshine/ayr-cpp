#ifndef AYR_FS_PATH_HPP
#define AYR_FS_PATH_HPP

#include <queue>

#include "oslib.h"
#include "../coro/Generator.hpp"
#include "../DynArray.hpp"

namespace ayr
{
	namespace fs
	{
#ifdef AYR_WIN
		constexpr char PATH_SEP = '\\';
#elif
		constexpr char PATH_SEP = '/';
#endif

		bool is_sep(char c) { return c == '\\' || c == '/'; }

		void system_error(const CString& path) { SystemError(std::format("{}: {}", get_error_msg(), path)); }

		// 拼接两个路径
		def join(const CString& path1, const CString& path2)
		{
			c_size len1 = path1.size();
			c_size len2 = path2.size();
			while (is_sep(path1[len1 - 1]))
				--len1;

			const char* s = path2.data();
			while (is_sep(*s))
				++s, --len2;

			return CString::cjoin(arr(vstr(path1.data(), len1), cstr(PATH_SEP), vstr(s, len2)));
		}

		void find_first_file_cb(const CString& path, const CString& pattern, const std::function<void(const WIN32_FIND_DATAA& find_data)>& callback)
		{
			WIN32_FIND_DATAA find_data;

			HANDLE handle = FindFirstFileA(join(path, pattern).c_str().c_str(), &find_data);

			ExTask task([&handle] { FindClose(handle); });

			if (handle == INVALID_HANDLE_VALUE)
				system_error(path);
			
			do
			{
				if (std::strcmp(find_data.cFileName, ".") == 0 || 
					std::strcmp(find_data.cFileName, "..") == 0)
					continue;

				callback(find_data);
			} while (FindNextFileA(handle, &find_data));
		}

		// 是否存在路径
		def exists(const CString& path)
		{
			return GetFileAttributesA(path.c_str().c_str()) != INVALID_FILE_ATTRIBUTES;
		}

		// 是否是文件
		def isfile(const CString& path)
		{
			DWORD attributes = GetFileAttributesA(path.c_str().c_str());

			return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		// 是否是文件夹
		def isdir(const CString& path)
		{
			return (GetFileAttributesA(path.c_str().c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		// 是否是绝对路径
		def isabs(const CString& path)
		{
			return (path.size() > 3 && path[1] == ':' && is_sep(path[2])) || 
				   (path.size() > 2 && is_sep(path[0]) && is_sep(path[1]));
		}

		// 获得绝对路径
		def abspath(const CString& path)
		{
			char full_path[MAX_PATH];
			DWORD len = GetFullPathNameA(path.c_str().c_str(), MAX_PATH, full_path, nullptr);

			if (len == 0 || len > MAX_PATH)
				SystemError(std::format("abspath failed for path: {}", path));

			return dstr(full_path, len);
		}

		// 获得当前路径
		def getcwd()
		{
			char current_path[MAX_PATH];
			DWORD len = GetCurrentDirectoryA(MAX_PATH, current_path);

			if (len == 0 || len > MAX_PATH)
				SystemError("getcwd failed");

			return dstr(current_path, len);
		}

		// 用当前路径创建一个文件夹
		def mkdir(const CString& path, bool exist_ok = false)
		{
			BOOL state = CreateDirectoryA(path.c_str().c_str(), nullptr);

			switch (state)
			{
			case ERROR_ALREADY_EXISTS:
				if (!exist_ok)
					system_error(path);
				break;
			case ERROR_PATH_NOT_FOUND:
				system_error(path);
				break;
			}
		}

		// 列出当前路径下的所有文件和文件夹
		def listdir(const CString& path)
		{
			DynArray<CString> results;
			find_first_file_cb(path, "*", [&results](const WIN32_FIND_DATAA& find_data) { results.append(find_data.cFileName); });
			return results.move_array();
		}

		// 删除当前路径的文件或文件夹
		def remove(const CString& path) -> void
		{
			DWORD attr = GetFileAttributesA(path.c_str().c_str());

			if (attr & FILE_ATTRIBUTE_READONLY)
				SetFileAttributesA(path.c_str().c_str(), attr & ~FILE_ATTRIBUTE_READONLY);

			if (attr != INVALID_FILE_ATTRIBUTES)
			{
				if (attr & FILE_ATTRIBUTE_DIRECTORY)
				{
					for (auto& sub_path : listdir(path))
						remove(join(path, sub_path));
					RemoveDirectoryA(path.c_str().c_str());
				}
				else
					DeleteFileA(path.c_str().c_str());
			}
		}

		// 遍历当前路径下的所有文件和文件夹
		def walk(const CString& path) -> coro::Generator<std::tuple<CString, Array<CString>, Array<CString>>>
		{
			DynArray<std::tuple<CString, Array<CString>, Array<CString>>> results;
			std::queue<CString> root_dirs;
			root_dirs.push(path);

			while (!root_dirs.empty())
			{
				CString root = std::move(root_dirs.front());
				root_dirs.pop();

				DynArray<CString> dirs, files;
				find_first_file_cb(root, "*", [&dirs, &files](const WIN32_FIND_DATAA& find_data) {
					if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
						dirs.append(dstr(find_data.cFileName));
					else
						files.append(dstr(find_data.cFileName));
					});
				co_yield std::make_tuple(std::move(root), dirs.move_array(), files.move_array());
			}
		}

		// 获得当前路径的文件或文件夹大小，单位为字节
		def filesize(const CString& path) -> uint64_t
		{
			uint64_t size_ = 0;
			if (isfile(path))
			{
				WIN32_FILE_ATTRIBUTE_DATA attr;
				if (!GetFileAttributesExA(path.c_str().c_str(), GetFileExInfoStandard, &attr))
					system_error(path);
				size_ = (static_cast<uint64_t>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
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
			for (c_size i = path.size() - 1; i >= 0; -- i)
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
			for (c_size i = path.size() - 1; i >= 0; -- i)
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
			for (c_size i = path.size() - 1; i >= 0; -- i)
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