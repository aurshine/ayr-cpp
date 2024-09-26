#ifndef AYR_FS_PATH_HPP
#define AYR_FS_PATH_HPP

#include <law/Printer.hpp>

#ifdef _WIN32

#include <cstdio>
#include <Windows.h>
#include <fileapi.h>


namespace ayr
{
	namespace fs
	{
		def exists(const char* path)
		{
			DWORD attributes = GetFileAttributesA(path);
			return (attributes != INVALID_FILE_ATTRIBUTES);
		}


		def isfile(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
		}


		def isdir(const char* path)
		{
			DWORD attr = GetFileAttributesA(path);
			return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
		}


		def mkdir(const char* path)
		{
			BOOL state = CreateDirectoryA(path, nullptr);

			switch (state)
			{
			case ERROR_ALREADY_EXISTS:
				RuntimeError(std::format("Directory already exists: {}", path));
			case ERROR_PATH_NOT_FOUND:
				FileNotFoundError(std::format("Path not found: {}", path));
			}
		}

		def mkdir(const CString& path) { mkdir(path.data()); }
	}
}
#elif __linux__


#else

#error "Unsupported platform"

#endif
#endif 