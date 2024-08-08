#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include <cstdio>
#include <atomic>

#include <law/Printer.hpp>
#include "Path.hpp"


namespace ayr
{
	namespace fs
	{
		class AyrFile
		{
		public:
			AyrFile(const char* filename, const char* mode) : file(std::fopen(filename, mode)), is_open(false)
			{
				if (file == nullptr)
				{
					if (!ayr::fs::isfile(filename))
						FileNotFoundError(std::format("file not found in {}", filename));
					else
						RuntimeError(std::format("failed to open file {}", filename));
				}
			}


			~AyrFile()
			{
				
			}

			std::FILE* file;

			std::atomic<bool> is_open;
		};
	}
}


#endif // AYR_FILE_HPP