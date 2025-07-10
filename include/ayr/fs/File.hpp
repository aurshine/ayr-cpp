#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include "oslib.h"

namespace ayr
{
	namespace fs
	{
		class AyrFile : public Object<AyrFile>
		{
			using self = AyrFile;

#if defined(AYR_WIN)
			using FD = HANDLE;

			constexpr static FD INVALID_FD = INVALID_HANDLE_VALUE;
#else
			using FD = int;

			constexpr static FD INVALID_FD = -1;
#endif
		public:
			AyrFile(const CString& filename, const CString& mode) : fd_(INVALID_FD)
			{
#if defined(AYR_WIN)
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

				fd_ = CreateFileA(
					filename.c_str().c_str(),
					dwDesiredAccess,
					FILE_SHARE_READ,
					nullptr,
					dwCreationDisposition,
					FILE_ATTRIBUTE_NORMAL,
					nullptr
				);

				if (fd_ == INVALID_HANDLE_VALUE)
					SystemError("Invalid HANDLE value, Failed to create or open file");

				if (mode == "a") SetFilePointer(fd_, 0, nullptr, FILE_END);
#else

				int flags = 0;
				if (mode == "w")
					flags = O_WRONLY | O_CREAT | O_TRUNC;
				else if (mode == "r")
					flags = O_RDONLY;
				else if (mode == "a")
					flags = O_WRONLY | O_CREAT | O_APPEND;
				else
					ValueError(std::format("Invalid value {}, that only support [w, r, a]", mode));

				fd_ = ::open(filename.c_str().c_str(), flags, 0666);

				if (fd_ == -1)
					SystemError("Failed to create or open file");
#endif
			}

			AyrFile(FD fd) : fd_(fd) {}

			AyrFile(self&& file) noexcept : fd_(file.fd_) { file.fd_ = INVALID_FD; }

			self& operator=(self&& file) noexcept
			{
				close();
				fd_ = file.fd_;
				file.fd_ = INVALID_FD;
				return *this;
			}

			~AyrFile() { close(); }

			void close()
			{
				if (fd_ != INVALID_FD)
				{
#if defined(AYR_WIN)
					CloseHandle(fd_);
#elif defined(AYR_LINUX)
					::close(fd_);
#endif
					fd_ = INVALID_FD;
				}
			}

			void write(const CString& data, c_size size = -1) const
			{
				if (size == -1) size = data.size();
#if defined(AYR_WIN)
				DWORD written_bytes = 0;
				BOOL ok = WriteFile(fd_, data.data(), size, &written_bytes, nullptr);
				if (!ok || written_bytes != size)
					SystemError("Failed to write from file");
#elif defined(AYR_LINUX)
				c_size num_written = 0;
				while (num_written < size)
				{
					c_size written_bytes = ::write(fd_, data.data() + num_written, size - num_written);
					if (written_bytes == -1)
						SystemError("Failed to write from file");
					num_written += written_bytes;
				}
#endif 
			}

			CString read() const
			{
				c_size buffer_size = file_size();
				char* buffer = ayr_alloc<char>(buffer_size);
#if defined(AYR_WIN)
				DWORD read_bytes = 0;
				BOOL ok = ReadFile(fd_, buffer, buffer_size, &read_bytes, nullptr);
				if (!ok || read_bytes != buffer_size)
					SystemError("Failed to read from file");

#elif defined(AYR_LINUX)
				c_size num_read = 0;
				while (num_read < buffer_size)
				{
					c_size read_bytes = ::read(fd_, buffer + num_read, buffer_size - num_read);
					if (read_bytes == -1)
						SystemError("Failed to read from file");
					num_read += read_bytes;
				}
#endif 
				return ostr(buffer, buffer_size);
			}

			c_size file_size() const
			{
#if defined(AYR_WIN)
				return GetFileSize(fd_, nullptr);
#elif defined(AYR_LINUX)
				struct stat st;
				if (::fstat(fd_, &st) == -1)
					SystemError("Failed to get file size");
				return st.st_size;
#endif
			}
		private:
			FD fd_;
		};
	}
}

#endif // AYR_FS_FILE_HPP