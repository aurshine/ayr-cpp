#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include "oslib.h"
#include "../coro/Generator.hpp"

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
			FD fd_;
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

			constexpr AyrFile(FD fd) : fd_(fd) {}

			constexpr AyrFile(self&& file) noexcept : fd_(file.fd_) { file.fd_ = INVALID_FD; }

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

			/*
			* @brief 写入指定长度数据到文件
			*
			* @param data 要写入的数据
			*
			* @param size 要写入数据的长度，默认为-1，表示写入整个数据
			*/
			void write(const CString& data, c_size size = -1) const
			{
				if (size == -1) size = data.size();
				// 已经写入的数据量
				c_size num_written = 0;
#if defined(AYR_WIN)
				DWORD written_bytes = 0;
				BOOL ok = 0;
#elif defined(AYR_LINUX)
				c_size written_bytes = 0;
				bool ok = false;
#endif

				while (num_written < size)
				{
#if defined(AYR_WIN)
					ok = WriteFile(fd_, data.data() + num_written, size - num_written, &written_bytes, nullptr);
#elif defined(AYR_LINUX)
					ok = ::write(fd_, data.data() + num_written, size - num_written) > 0;
#endif 
					if (!ok)
						SystemError("Failed to write from file");
					num_written += written_bytes;
				}
			}

			/*
			* @brief 将文件内容读入缓冲区
			* 
			* @param buffer 要读入的缓冲区
			* 
			* @param size 要读入数据的长度，默认为-1，表示读取整个文件
			* 
			* @return 返回缓冲区
			*/
			Buffer& read_buffer(Buffer& buffer, c_size size = -1) const
			{
				size = ifelse(size > 0, size, file_size());
				// buffer容量不够
				if (size > buffer.writeable_size())
				{
					Buffer new_buffer(size + buffer.readable_size());
					new_buffer.append_bytes(buffer.peek(), buffer.readable_size());
					buffer = std::move(new_buffer);
				}

#if defined(AYR_WIN)
				DWORD read_bytes = 0;
				BOOL ok = 0;
#elif defined(AYR_LINUX)
				c_size read_bytes = 0;
				bool ok = false;
#endif
				while (buffer.writeable_size() > 0)
				{
#if defined(AYR_WIN)
					ok = ReadFile(fd_, buffer.write_ptr(), buffer.writeable_size(), &read_bytes, nullptr);
#elif defined(AYR_LINUX)
					ok = ::read(fd_, buffer.write_ptr(), buffer.writeable_size()) >= 0;
#endif
					// 读完了
					if (read_bytes == 0) break;
					if (!ok)
						SystemError("Failed to read from file");
					buffer.written(read_bytes);
				}
				return buffer;
			}

			/*
			* @brief 将文件内容读入缓冲区
			* 
			* @param size 要读入数据的长度，默认为-1，表示读取整个文件
			* 
			* @return 返回缓冲区
			*/
			Buffer read_buffer(c_size size = -1) const
			{
				size = ifelse(size > 0, size, file_size());
				Buffer buffer(size);
				return read_buffer(buffer, size);
			}

			/*
			* @brief 读取文件指定长度数据
			*
			* @param size 要读取数据的长度，默认为-1，表示读取整个文件
			*
			* @return 返回读取的数据
			*/
			CString read(c_size size = -1) const { return from_buffer(read_buffer(size)); }
			
			/*
			* @brief 按行读取文件内容
			*
			* @detail 每行字符串末尾不包含换行符
			* 
			* @return 返回一个协程生成器
			*/
			coro::Generator<CString> readlines() const
			{
				constexpr c_size BLOCK_SIZE = 1024;

				Buffer buffer(BLOCK_SIZE);
				// 调用read_buffer前, 读缓冲区的大小
				c_size before_read_size = 0;
				// sep 的位置
				c_size eol_pos = 0;
				// 读到文件末尾
				bool eof = false;

				do {
					read_buffer(buffer, BLOCK_SIZE);
					// 判断是否读完了
					eof = before_read_size == buffer.readable_size();
					
					/*
					* 当需要read_buffer时，表示前befor_read_size都没有换行符
					* 
					* 第一次获取eol_pos时，从befor_read_size开始查找
					* 
					* 后续获取eol_pos时，下标0开始查找
					*/
					for (eol_pos = buffer.find_eol(before_read_size); eol_pos != -1; eol_pos = buffer.find_eol())
					{
						co_yield dstr(buffer.peek(), eol_pos);
						buffer.retrieve(eol_pos + 1);
					}
					before_read_size = buffer.readable_size();
				} while (!eof);

				// 最后一行
				if (buffer.readable_size() > 0)
					co_return from_buffer(std::move(buffer));
			}

			/*
			* @brief 按行写入文件内容
			*
			* @detail 每行字符串末尾包含换行符
			*
			* @param obj 要写入的字符串可迭代对象
			*/
			template<IteratableV<CString> Obj>
			void writelines(Obj&& obj) const { write(vstr("\n").join(obj)); }

			// 读取文件大小
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
		};
	}
}

#endif // AYR_FS_FILE_HPP