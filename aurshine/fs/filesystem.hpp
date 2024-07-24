#pragma ocne

#ifndef _WIN32
#error "This file is only for Windows"
#endif

#include <Windows.h>
#include <fileapi.h>

#include <law/Printer.hpp>


namespace ayr
{
	namespace fs
	{
		inline void mkdir(const char* path)
		{
			BOOL ret = CreateDirectoryA(path, nullptr);

			switch (ret)
			{
				case ERROR_ALREADY_EXISTS:
					RuntimeError("Directory already exists: {}", path);
					break;
				case ERROR_PATH_NOT_FOUND:
					RuntimeError("Path not found: {}", path);
					break;
			}
		}
	}
}