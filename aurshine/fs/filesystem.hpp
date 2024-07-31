#pragma once
#include <format>
#include <law/Printer.hpp>

#ifdef _WIN32

#include <cstdio>
#include <Windows.h>
#include <fileapi.h>

namespace ayr
{
	namespace fs
	{
		der(bool) exists(const char* path)
		{
			DWORD attributes = GetFileAttributesA(path);
			return (attributes != INVALID_FILE_ATTRIBUTES);
		}


		der(bool) isfile(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
		}


		der(bool) isdir(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}


		der(void) mkdir(const char* path, bool exist_ok = false)
		{
			BOOL ret = CreateDirectoryA(path, nullptr);

			switch (ret)
			{
			case ERROR_ALREADY_EXISTS:
				if (exist_ok) break;
				RuntimeError(std::format("Directory already exists: {}", path));
			case ERROR_PATH_NOT_FOUND:
				RuntimeError(std::format("Path not found: {}", path));
			}
		}

		der(void) mkdir(const CString& path) { mkdir(path.str); }

		class Open : public Object
		{
		public:
			Open(const char* path, const char* mode)
			{

			}
		};
	}
}
#elif __linux__


#else

#error "Unsupported platform"

#endif