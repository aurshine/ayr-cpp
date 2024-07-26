#pragma once
#include <format>
#include <law/Printer.hpp>

#ifdef _WIN32

#include <Windows.h>
#include <fileapi.h>

namespace ayr
{
	namespace fs
	{
		def exists(const char* path) -> bool
		{
			DWORD attributes = GetFileAttributesA(path);
			return (attributes != INVALID_FILE_ATTRIBUTES);
		}


		def isfile(const char* path) -> bool
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
		}


		def isdir(const char* path) -> bool
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}


		def mkdir(const char* path, bool exist_ok = false) -> void
		{
			BOOL ret = CreateDirectoryA(path, nullptr);

			switch (ret)
			{
			case ERROR_ALREADY_EXISTS:
				if (exist_ok) break;
				RuntimeError(std::format("Directory already exists: {}", path));
				break;
			case ERROR_PATH_NOT_FOUND:
				RuntimeError(std::format("Path not found: {}", path));
				break;
			}
		}

		def mkdir(const CString& path) -> void { mkdir(path.str); }
	}
}
#elif __linux__


#else

#error "Unsupported platform"

#endif