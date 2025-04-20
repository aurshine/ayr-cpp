#ifndef AYR_FS_WIN_FILE_HPP
#define AYR_FS_WIN_FILE_HPP

#include <cstdio>
#include <atomic>
#include <memory>

#include "Path.hpp"


namespace ayr
{
	namespace fs
	{
		class AyrFile : public Object<AyrFile>
		{
			using self = AyrFile;
		public:
			AyrFile(const char* filename, CString mode): fh(INVALID_HANDLE_VALUE)
			{
				int dwDesiredAccess = 0, dwCreationDisposition = 0;
				if (mode == "w")
				{
					dwDesiredAccess = GENERIC_WRITE;
					dwCreationDisposition = CREATE_ALWAYS;
				}
				else if (mode == "r")
				{
					dwDesiredAccess = GENERIC_READ;
					dwCreationDisposition = OPEN_EXISTING;
				}
				else if (mode == "a")
				{
					dwDesiredAccess = FILE_APPEND_DATA;
					dwCreationDisposition = OPEN_ALWAYS;
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
				
				if (mode == "a") SetFilePointer(fh, 0, nullptr, FILE_END);
			}

			AyrFile(self&& file) noexcept : fh(file.fh) { file.fh = nullptr; }

			self& operator=(self&& file) noexcept
			{
				close();
				return *ayr_construct(this, std::move(file));
			}

			~AyrFile() { close(); }

			void close() 
			{ 
				if (fh != INVALID_HANDLE_VALUE)
				{
					CloseHandle(fh);
					fh = nullptr;
				}
			}

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