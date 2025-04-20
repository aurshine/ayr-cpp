#ifndef AYR_FS_LINUX_FILE_HPP
#define AYR_FS_LINUX_FILE_HPP

#include "Path.hpp"

namespace ayr
{
	namespace fs
	{
		class AyrFile : public Object<AyrFile>
		{
			using self = AyrFile;

			int fd;
		public:
			AyrFile(const char* path, CString mode) : fd(-1)
			{
				int flags = 0;
				if (mode[0] == 'r')
					flags = O_RDONLY;
				else if (mode[0] == 'w')
					flags = O_WRONLY | O_CREAT | O_TRUNC;
				else if (mode[0] == 'a')
					flags = O_WRONLY | O_CREAT | O_APPEND;
				else
					RuntimeError("Invalid file mode");

			}
		};
	}
}
#endif