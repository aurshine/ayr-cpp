#ifndef AYR_FS_WIN_PATH_HPP
#define AYR_FS_WIN_PATH_HPP

#include <ayr/detail/Printer.hpp>

#include "winlib.hpp"

namespace ayr
{
	namespace fs
	{
		constexpr char PATH_SEP = '\\';

		// 判断文件或文件夹是否存在
		def exists(const char* path)
		{
			DWORD attributes = GetFileAttributesA(path);
			return (attributes != INVALID_FILE_ATTRIBUTES);
		}

		// 判断是否为文件
		def isfile(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
		}

		// 判断是否为文件夹
		def isdir(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}

		// 创建一个文件夹
		def mkdir(const char* path, bool exist_ok = false)
		{
			BOOL state = CreateDirectoryA(path, nullptr);

			switch (state)
			{
			case ERROR_ALREADY_EXISTS:
				if (!exist_ok)
					RuntimeError(std::format("Directory already exists: {}", path));
				break;
			case ERROR_PATH_NOT_FOUND:
				FileNotFoundError(std::format("Path not found: {}", path));
				break;
			}
		}

		// 删除一个文件或文件夹
		def remove(const char* path)
		{
			if (!exists(path))
				FileNotFoundError(std::format("File or Directory not found in {}", path));

			if (isfile(path) && !DeleteFileA(path))
				SystemError("Failed to remove file");
			else if (isdir(path) && !RemoveDirectoryA(path))
				SystemError("Failed to remove directory");
		}

		// 拼接两个路径
		def join(const char* path1, const char* path2) -> CString
		{
			c_size len1 = strlen(path1);
			c_size len2 = strlen(path2);
			c_size len = len1 + len2 + (path1[len1 - 1] != '\\' && path1[len1 - 1] != '/');

			CString result{ len };
			std::memcpy(result.data(), path1, len1);

			if (len1 + len2 != len)
				result[len1] = PATH_SEP;

			std::memcpy(result.data() + len - len2, path2, len2);
			return result;
		}

		// 列出文件夹中的文件
		def listdir(const char* path) -> DynArray<CString>
		{
			WIN32_FIND_DATAA find_data;
			HANDLE handle = FindFirstFileA(join(path, "*"), &find_data);

			if (handle == INVALID_HANDLE_VALUE)
				SystemError(std::format("Failed to list directory: {}", path));

			DynArray<CString> results;
			do
			{
				if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
					continue;
				results.append(find_data.cFileName);
			} while (FindNextFileA(handle, &find_data));

			FindClose(handle);
			return results;
		}
	}
}

#endif 