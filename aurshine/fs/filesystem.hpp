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
				case ERROR_PATH_NOT_FOUND:
					RuntimeError("Path not found: {}", path);
				case ERROR_ACCESS_DENIED:
					RuntimeError("Access denied: {}", path);
				case ERROR_INVALID_NAME:
					RuntimeError("Invalid name: {}", path);
				case ERROR_FILENAME_EXCED_RANGE:
					RuntimeError("Filename too long: {}", path);
				case ERROR_INVALID_PARAMETER:
					RuntimeError("Invalid parameter: {}", path);
				case ERROR_NOT_ENOUGH_MEMORY:
					RuntimeError("Not enough memory: {}", path);
				case ERROR_INVALID_DRIVE:
					RuntimeError("Invalid drive: {}", path);
				case ERROR_DIR_NOT_EMPTY:
					RuntimeError("Directory not empty: {}", path);
				case ERROR_FILE_EXISTS:
					RuntimeError("File already exists: {}", path);
				case ERROR_OPERATION_ABORTED:
					RuntimeError("Operation aborted: {}", path);
				case ERROR_NOT_SUPPORTED:
					RuntimeError("Not supported: {}", path);
				case ERROR_DISK_FULL:
					RuntimeError("Disk full: {}", path);
			}
		}
	}
}