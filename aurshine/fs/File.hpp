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
			AyrFile(CString filename, CString mode) : file(std::fopen(filename, mode)), is_open(false)
			{
				if (file == nullptr)
				{
					if (!fs::isfile(filename))
						FileNotFoundError(std::format("file not found in {}", filename));
					else
						RuntimeError(std::format("failed to open file {}", filename));
				}
			}

			void close()
			{
				if (is_open)
				{
					std::fclose(file);
					is_open = false;
				}
			}

			~AyrFile()
			{
				close();
			}

			std::FILE* file;

			std::atomic<bool> is_open;
		};
	}
}


#endif // AYR_FILE_HPP