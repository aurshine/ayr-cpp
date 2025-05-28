#ifndef AYR_FS_PATH_HPP
#define AYR_FS_PATH_HPP

#include "oslib.h"

#if defined(AYR_WIN)
#include "../DynArray.hpp"

#include <queue>
namespace ayr
{
	namespace fs
	{
		constexpr char PATH_SEP = '\\';

		bool is_sep(char c) { return c == '\\' || c == '/'; }

		void system_error(const char* path) { SystemError(std::format("{}: {}", get_error_msg(), path)); }

		// 拼接两个路径
		def join(const char* path1, const char* path2)
		{
			c_size len1 = strlen(path1);
			c_size len2 = strlen(path2);
			if (is_sep(path1[len1 - 1]))
				--len1;
			if (is_sep(path2[0]))
				++path2, --len2;

			CString result{ len1 + len2 + 1 };
			std::memcpy(result.data(), path1, len1);
			std::memcpy(result.data() + len1, &PATH_SEP, 1);
			std::memcpy(result.data() + len1 + 1, path2, len2);
			return result;
		}

		void find_first_file_cb(const char* path, const char* pattern, const std::function<void(const WIN32_FIND_DATAA& find_data)>& callback)
		{
			WIN32_FIND_DATAA find_data;

			HANDLE handle = FindFirstFileA(join(path, pattern), &find_data);

			if (handle == INVALID_HANDLE_VALUE)
				system_error(path);
			do
			{
				if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
					continue;

				callback(find_data);
			} while (FindNextFileA(handle, &find_data));

			FindClose(handle);
		}

		// 是否存在路径
		def exists(const char* path)
		{
			return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
		}

		// 是否是文件
		def isfile(const char* path)
		{
			DWORD attributes = GetFileAttributesA(path);

			return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		// 是否是文件夹
		def isdir(const char* path)
		{
			return (GetFileAttributesA(path) & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		// 是否是绝对路径
		def isabs(const char* path)
		{
			return (path[0] && path[1] == ':' && is_sep(path[2])) || (is_sep(path[0]) && is_sep(path[1]));
		}

		// 获得绝对路径
		def abspath(const char* path)
		{
			char full_path[MAX_PATH];
			DWORD result = GetFullPathNameA(path, MAX_PATH, full_path, nullptr);

			if (result == 0 || result > MAX_PATH)
				SystemError(std::format("abspath failed for path: {}", path));

			return CString(full_path, result);
		}

		def getcwd()
		{
			char current_path[MAX_PATH];
			DWORD result = GetCurrentDirectoryA(MAX_PATH, current_path);

			if (result == 0 || result > MAX_PATH)
				SystemError("getcwd failed");

			return CString(current_path, result);
		}

		// 用当前路径创建一个文件夹
		def mkdir(const char* path, bool exist_ok = false)
		{
			BOOL state = CreateDirectoryA(path, nullptr);

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
		def listdir(const char* path)
		{
			DynArray<CString> results;
			find_first_file_cb(path, "*", [&results](const WIN32_FIND_DATAA& find_data) { results.append(find_data.cFileName); });
			return results.to_array();
		}

		// 删除当前路径的文件或文件夹
		def remove(const char* path) -> void
		{
			DWORD attr = GetFileAttributesA(path);

			if (attr & FILE_ATTRIBUTE_READONLY)
				SetFileAttributesA(path, attr & ~FILE_ATTRIBUTE_READONLY);

			if (attr != INVALID_FILE_ATTRIBUTES)
			{
				if (attr & FILE_ATTRIBUTE_DIRECTORY)
				{
					for (auto& sub_path : listdir(path))
						remove(join(path, sub_path));
					RemoveDirectoryA(path);
				}
				else
					DeleteFileA(path);
			}
		}

		// 遍历当前路径下的所有文件和文件夹
		def walk(const char* path)
		{
			DynArray<std::tuple<CString, Array<CString>, Array<CString>>> results;
			std::queue<CString> root_dirs;
			root_dirs.push(path);

			while (!root_dirs.empty())
			{
				CString root = root_dirs.front();
				root_dirs.pop();

				DynArray<CString> dirs, files;
				find_first_file_cb(root, "*", [&dirs, &files](const WIN32_FIND_DATAA& find_data) {
					if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
						dirs.append(find_data.cFileName);
					else
						files.append(find_data.cFileName);
					});
				results.append(std::make_tuple(std::move(root), dirs.to_array(), files.to_array()));
			}
			return results;
		}

		// 获得当前路径的文件或文件夹大小，单位为字节
		def filesize(const char* path) -> uint64_t
		{
			uint64_t size_ = 0;
			if (isfile(path))
			{
				WIN32_FILE_ATTRIBUTE_DATA attr;
				if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr))
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
		def basename(const char* path)
		{
			c_size last_sep_pos = -1, pos = 0;
			while (path[pos])
			{
				if (is_sep(path[pos])) last_sep_pos = pos;
				++pos;
			}

			return CString(path + last_sep_pos + 1, pos - last_sep_pos - 1);
		}

		// 获得路径的目录
		def dirname(const char* path)
		{
			c_size last_sep_pos = 0, pos = 0;
			while (is_sep(path[pos])) ++pos, ++last_sep_pos;
			while (path[pos])
			{
				if (is_sep(path[pos])) last_sep_pos = pos;
				++pos;
			}

			return CString(path, last_sep_pos);
		}

		// 获得路径的目录和文件名
		def split(const char* path)
		{
			return std::make_pair(dirname(path), basename(path));
		}

		// 获得文件扩展名
		def splitext(const char* path)
		{
			c_size last_dot_pos = -1, pos = 0;
			while (path[pos] == '.') ++pos;
			while (path[pos])
			{
				if (path[pos] == '.') last_dot_pos = pos;
				++pos;
			}

			if (last_dot_pos == -1) last_dot_pos = pos;
			return std::make_pair(CString(path, last_dot_pos), CString(path + last_dot_pos, pos - last_dot_pos));
		}
	}
}
#elif defined(AYR_LINUX)
#endif 
#endif // AYR_FS_PATH_HPP