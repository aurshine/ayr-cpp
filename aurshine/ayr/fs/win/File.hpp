#ifndef AYR_FS_WIN_FILE_HPP
#define AYR_FS_WIN_FILE_HPP

#include <cstdio>
#include <atomic>
#include <memory>

#include <ayr/DynArray.hpp>
#include <ayr/detail/NoCopy.hpp>
#include "Path.hpp"


namespace ayr
{
	namespace fs
	{
		class AyrFile : public Object<AyrFile>, public NoCopy
		{
			using self = AyrFile;
		public:
			AyrFile(const char* filename, CString mode)
			{
				int dwDesiredAccess = 0, dwCreationDisposition = 0;
				if (mode == cstr("w"))
				{
					dwDesiredAccess = GENERIC_WRITE;
					dwCreationDisposition = CREATE_ALWAYS;
				}
				else if (mode == cstr("r"))
				{
					dwDesiredAccess = GENERIC_READ;
					dwCreationDisposition = OPEN_EXISTING;
				}
				else if (mode == cstr("a"))
				{
					dwDesiredAccess = FILE_APPEND_DATA;
					dwCreationDisposition = OPEN_ALWAYS;
					SetFilePointer(fh, 0, nullptr, FILE_END);
				}
				else
					ValueError(std::format("Invalid value {}, that only support [w, r, a]", mode));

				fh = CreateFileA(
					filename,
					dwDesiredAccess,
					FILE_SHARE_READ,
					nullptr,
					dwCreationDisposition,
					FILE_ATTRIBUTE_NORMAL,
					nullptr
				);

				if (fh == INVALID_HANDLE_VALUE)
					SystemError("Invalid HANDLE value, Failed to create or open file");
			}

			AyrFile(self&& file) noexcept : fh(file.fh) { file.fh = nullptr; }

			~AyrFile() { close(); }

			void close() { CloseHandle(fh); fh = nullptr; }

			void write(const char* c) const
			{
				DWORD written_bytes = 0;
				if (!WriteFile(fh, c, strlen(c), &written_bytes, nullptr))
					SystemError("Failed to write from file");
			}

			CString read() const
			{
				DWORD read_bytes = 0;
				c_size buffer_size = file_size() + 1;
				CString buffer{ buffer_size };

				if (!ReadFile(fh, buffer.data(), buffer_size, &read_bytes, nullptr))
					SystemError("Failed to read from file");

				return buffer;
			}

			c_size file_size() const { return GetFileSize(fh, nullptr); }

		private:
			HANDLE fh;
		};
	}
}


#endif // AYR_FILE_HPP