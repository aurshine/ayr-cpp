#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include "oslib.h"
#include "../coro/Generator.hpp"

namespace ayr
{
	namespace fs
	{
		class AyrFile
		{
			using self = AyrFile;

#if defined(AYR_WIN)
			using FD = HANDLE;

			constexpr static FD INVALID_FD = INVALID_HANDLE_VALUE;

			constexpr static const char* SYS_EOL = "\r\n";
#else
			using FD = int;

			constexpr static FD INVALID_FD = -1;

			constexpr static const char* SYS_EOL = "\n";
#endif
			FD fd_;

#if defined(AYR_WIN)
			static Buffer atring_to_wide(const Atring& value)
			{
				if (value.size() > INT_MAX)
					ValueError("Atring path is too long to convert to Windows UTF-16");

				Buffer result((value.size() * 2 + 1) * sizeof(wchar_t));
				wchar_t* output = reinterpret_cast<wchar_t*>(result.write_ptr());
				c_size output_size = 0;
				for (const AChar& ch : value)
				{
					uint32_t code = ch.ord();
					if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
						EncodingError(ayr::format("Invalid Unicode code point in Windows path: {}", code));

					if (code <= 0xFFFF)
						output[output_size++] = static_cast<wchar_t>(code);
					else
					{
						code -= 0x10000;
						output[output_size++] = static_cast<wchar_t>(0xD800 + (code >> 10));
						output[output_size++] = static_cast<wchar_t>(0xDC00 + (code & 0x3FF));
					}
				}
				output[output_size++] = L'\0';
				result.written(output_size * sizeof(wchar_t));
				return result;
			}
#endif

		public:
			/**
			* @brief AyrFile构造函数
			*
			* @param filename 文件名字节串；Windows 下交给当前 ANSI 代码页 API 解释。
			*
			* @param mode 打开模式：`w` 覆盖写入、`r` 只读、`a` 追加写入。
			*
			* @throws AyrError mode 无效或系统无法打开文件时抛出。
			*/
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
					ValueError(ayr::format("Invalid value {}, that only support [w, r, a]", mode));

				fd_ = CreateFileA(
					filename.c_str(),
					dwDesiredAccess,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					nullptr,
					dwCreationDisposition,
					FILE_ATTRIBUTE_NORMAL,
					nullptr
				);

				if (fd_ == INVALID_FD)
					SystemError(ayr::format("Failed to create or open file {} with mode {}, {}",filename, mode, get_system_error_msg()));
#else

				int flags = 0;
				if (mode == "w")
					flags = O_WRONLY | O_CREAT | O_TRUNC;
				else if (mode == "r")
					flags = O_RDONLY;
				else if (mode == "a")
					flags = O_WRONLY | O_CREAT | O_APPEND;
				else
					ValueError(ayr::format("Invalid value {}, that only support [w, r, a]", mode));

				fd_ = ::open(filename.c_str(), flags, 0666);

				if (fd_ == -1)
					SystemError(ayr::format("Failed to create or open file {} with mode {}, {}",filename, mode, get_system_error_msg()));
#endif
			}

			/**
			 * @brief 从已有系统文件描述符构造拥有所有权的 AyrFile。
			 * 
			 * @param fd 调用者转交给 AyrFile 管理的文件描述符或句柄。
			 * 
			 * @note 对象销毁时会关闭 fd；调用者不得再独立关闭同一 fd。
			 */
			constexpr AyrFile(FD fd) : fd_(fd) {}

			/**
			 * @brief 移动构造文件对象并转移底层句柄所有权。
			 * 
			 * @param other 被移动对象；完成后其句柄变为无效值。
			 */
			constexpr AyrFile(self&& other) noexcept : fd_(other.fd_) { other.fd_ = INVALID_FD; }

			self& operator=(self&& other)
			{
				if (this == &other) return *this;

				close();
				fd_ = other.fd_;
				other.fd_ = INVALID_FD;
				return *this;
			}

			~AyrFile() { close(); }

			/**
			 * @brief 显式关闭文件句柄。
			 * 
			 * @note 重复关闭已关闭对象不会执行任何操作。
			 * 
			 * @throws AyrError 系统关闭操作失败时抛出，消息包含 get_system_error_msg()。
			 */
			void close()
			{
				if (fd_ == INVALID_FD) return;
#if defined(AYR_WIN)
				if (!CloseHandle(fd_))
					SystemError(ayr::format("Failed to close file, {}", get_system_error_msg()));
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				if (::close(fd_) != 0)
					SystemError(ayr::format("Failed to close file, {}", get_system_error_msg()));
#endif
				fd_ = INVALID_FD;
			}

			/**
			* @brief 将文件内容读入缓冲区
			*
			* @param buffer 要读入的缓冲区
			*
			* @param size 要读入数据的长度；-1 表示读取当前位置到文件末尾，0 表示不读取。
			*
			* @return 实际读取的字节数
			*
			* @throws AyrError size 小于 -1、扩容失败或系统读取失败时抛出。
			*/
			c_size read(Buffer& buffer, c_size size = -1) const
			{
				if (size == 0)
					return 0;
				if (size < 0)
					size = remaining_size();
				
				buffer.adjust_util(size);
				c_size total_read = 0;
#if defined(AYR_WIN)
				DWORD read_bytes = 0;
				BOOL ok = 0;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				ssize_t read_bytes = 0;
#endif
				while (total_read < size)
				{
#if defined(AYR_WIN)
					DWORD request_size = std::min<c_size>(MAXDWORD, size - total_read);
					ok = ReadFile(fd_, buffer.write_ptr(), request_size, &read_bytes, nullptr);
					if (!ok)
						SystemError(ayr::format("Failed to read from file, {}", get_system_error_msg()));
#elif defined(AYR_LINUX) || defined(AYR_MAC)
					size_t request_size = std::min<c_size>(SSIZE_MAX, size - total_read);
					do
					{
						read_bytes = ::read(fd_, buffer.write_ptr(), request_size);
					} while (read_bytes < 0 && errno == EINTR);
					if (read_bytes < 0)
						SystemError(ayr::format("Failed to read from file, {}", get_system_error_msg()));
#endif
					// 读完了
					if (read_bytes == 0) break;
					total_read += read_bytes;
					buffer.written(read_bytes);
				}
				return total_read;
			}

			/**
			* @brief 读取文件指定长度数据
			*
			* @param size 要读取数据的长度；-1 表示读取当前位置到文件末尾。
			*
			* @return 返回读取的数据
			*
			* @throws AyrError size 无效或读取失败时抛出。
			*/
			CString read(c_size size = -1) const 
			{ 
				Buffer buffer;
				read(buffer, size);
				return from_buffer(std::move(buffer));
			}

			/**
			* @brief 按行读取文件内容
			*
			* @details 同时识别 LF 和 CRLF，每行字符串末尾不包含换行符。
			*
			* @return 返回一个协程生成器, 生成器 yield 的 CString 是视图
			*
			* @note 生成器持有复制出的文件句柄，因此可安全用于临时 AyrFile。
			* 
			* @throws AyrError 复制句柄或读取文件失败时抛出。
			*/
			coro::Generator<CString> readlines() const
			{ 
				constexpr c_size BLOCK_SIZE = 1024;

				Buffer buffer(BLOCK_SIZE);
				c_size before_read_size = 0;
				c_size eol_pos = 0;
				c_size num_read = 0;

				do {
					num_read = read(buffer, BLOCK_SIZE);
					for (eol_pos = buffer.find_eol(before_read_size); eol_pos != -1; eol_pos = buffer.find_eol())
					{
						// 同时接受 LF 和 CRLF，返回值中不保留换行字符。
						if (eol_pos > 0 && *(buffer.peek() + eol_pos - 1) == '\r')
							co_yield vstr(buffer.peek(), eol_pos - 1);
						else
							co_yield vstr(buffer.peek(), eol_pos);

						// 先 yield 再 retrieve，可以安全使用 vstr 视图。
						buffer.retrieve(eol_pos + 1);
					}
					before_read_size = buffer.readable_size();
				} while (num_read == BLOCK_SIZE);

				if (buffer.readable_size() > 0)
					co_return from_buffer(std::move(buffer));
			}

			/**
			* @brief 写入指定长度数据到文件
			*
			* @param data 要写入的数据
			*
			* @throws AyrError 系统写入失败时抛出。
			*/
			void write(const CString& data) const
			{
				c_size size = data.size();

				// 已经写入的数据量
				c_size num_written = 0;
#if defined(AYR_WIN)
				DWORD written_bytes = 0;
				BOOL ok = 0;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				ssize_t written_bytes = 0;
#endif

				while (num_written < size)
				{
#if defined(AYR_WIN)
					DWORD request_size = std::min<c_size>(MAXDWORD, size - num_written); 
					ok = WriteFile(fd_, data.data() + num_written, request_size, &written_bytes, nullptr);
					if (!ok)
						SystemError(ayr::format("Failed to write to file, {}", get_system_error_msg()));
#elif defined(AYR_LINUX) || defined(AYR_MAC)
					size_t request_size = std::min<c_size>(SSIZE_MAX, size - num_written);
					do
					{
						written_bytes = ::write(fd_, data.data() + num_written, request_size);
					} while (written_bytes < 0 && errno == EINTR);
					if (written_bytes < 0)
						SystemError(ayr::format("Failed to write to file, {}", get_system_error_msg()));
#endif
					if (written_bytes == 0)
						SystemError("Failed to write to file: system call wrote zero bytes");
					num_written += written_bytes;
				}
			}

			/**
			* @brief 写入指定长度数据到文件
			*
			* @param buffer 要写入的数据
			*
			* @param size 要写入数据的长度；-1 表示写入全部可读数据。
			*
			* @note 成功写入后会从 buffer 中消费对应字节。
			* @throws AyrError size 无效、超过可读长度或系统写入失败时抛出。
			*/
			void write(Buffer& buffer, c_size size = -1)
			{
				write(vstr(buffer.peek(), size));
				buffer.retrieve(size);
			}

			/**
			* @brief 按行写入文件内容
			*
			* @details 每个元素后写入当前平台换行符，包括最后一个元素。
			*
			* @param obj 要写入的字符串可迭代对象
			*
			* @throws AyrError 任意一次写入失败时抛出。
			*/
			template<IteratableV<CString> Obj>
			void writelines(Obj&& obj) const
			{
				for (const CString& line : obj)
				{
					write(line);
					write(SYS_EOL);
				}
			}

			/**
			 * @brief 查询当前已打开文件的总字节数。
			 * 
			 * @return 文件总大小，与当前文件偏移无关。
			 * 
			 * @throws AyrError 系统查询失败或返回负数大小时抛出。
			 */
			c_size file_size() const
			{
#if defined(AYR_WIN)
				LARGE_INTEGER size{};
				if (!GetFileSizeEx(fd_, &size))
					SystemError(ayr::format("Failed to get file size, {}", get_system_error_msg()));
				if (size.QuadPart < 0)
					SystemError("Failed to get file size: system returned a negative size");
				return size.QuadPart;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				struct stat st;
				if (::fstat(fd_, &st) == -1)
					SystemError(ayr::format("Failed to get file size, {}", get_system_error_msg()));
				if (st.st_size < 0)
					SystemError("Failed to get file size: system returned a negative size");
				return st.st_size;
#endif
			}

			/**
			 * @brief 复制底层系统文件句柄，使新对象独立负责关闭复制出的句柄。
			 *
			 * @return 持有复制句柄的 AyrFile。
			 *
			 * @note 复制句柄与原句柄共享当前文件偏移。
			 *
			 * @throws AyrError 系统无法复制句柄时抛出。
			 */
			self duplicate() const
			{
#if defined(AYR_WIN)
				HANDLE duplicate_fd = INVALID_FD;
				if (!DuplicateHandle(
					GetCurrentProcess(),
					fd_,
					GetCurrentProcess(),
					&duplicate_fd,
					0,
					FALSE,
					DUPLICATE_SAME_ACCESS
				))
					SystemError(ayr::format("Failed to duplicate file handle, {}", get_system_error_msg()));
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				int duplicate_fd = INVALID_FD;
				do
				{
					duplicate_fd = ::dup(fd_);
				} while (duplicate_fd == INVALID_FD && errno == EINTR);
				if (duplicate_fd == INVALID_FD)
					SystemError(ayr::format("Failed to duplicate file descriptor, {}", get_system_error_msg()));
#endif
			return self(duplicate_fd);
			}
		private:
			/**
			* @brief 计算从当前文件偏移到文件末尾的剩余字节数。
			* 
			* @return 剩余字节数；当前位置超过文件末尾时返回 0。
			* 
			* @throws AyrError 获取文件大小或当前偏移失败时抛出。
			*/
			c_size remaining_size() const
			{
				c_size total_size = file_size();
#if defined(AYR_WIN)
				LARGE_INTEGER zero{}, current{};
				if (!SetFilePointerEx(fd_, zero, &current, FILE_CURRENT))
					SystemError(ayr::format("Failed to get current file position, {}", get_system_error_msg()));
				return ifelse(current.QuadPart < total_size, total_size - current.QuadPart, 0);
#elif defined(AYR_LINUX) || defined(AYR_MAC)
				off_t current = ::lseek(fd_, 0, SEEK_CUR);
				if (current == static_cast<off_t>(-1))
					SystemError(ayr::format("Failed to get current file position, {}", get_system_error_msg()));
				return ifelse(current < total_size, total_size - current, 0);
#endif
			}
		};

		/**
		* @brief 读取文件内容
		*
		* @param filename 文件名
		*
		* @return 从文件起始位置读取的全部内容。
		* 
		* @throws AyrError 打开或读取文件失败时抛出。
		*/
		def read(const CString& filename) { return AyrFile(filename, "r").read(); }

		/**
		* @brief 读取文件所有内容
		*
		* @param filename 文件名
		*
		* @param buffer 要读入的缓冲区
		*
		* @return 实际追加到 buffer 的字节数。
		* 
		* @throws AyrError 打开或读取文件失败时抛出。
		*/
		def read(const CString& filename, Buffer& buffer) { return AyrFile(filename, "r").read(buffer); }

		/**
		* @brief 写入文件内容
		*
		* @param filename 文件名
		*
		* @param data 要写入的数据
		* 
		* @throws AyrError 打开失败或写入失败时抛出。
		*/
		def write(const CString& filename, const CString& data) { return AyrFile(filename, "w").write(data); }

		/**
		* @brief 写入文件内容
		*
		* @param filename 文件名
		*
		* @param buffer 要写入的数据
		*
		* @param size 要写入数据的长度；-1 表示写入 buffer 全部可读数据。
		* 
		* @note 成功后从 buffer 中消费已写入字节。
		* 
		* @throws AyrError 打开失败或写入失败时抛出。
		*/
		def write(const CString& filename, Buffer& buffer, c_size size = -1) { return AyrFile(filename, "w").write(buffer, size); }

		/**
		* @brief 按行写入文件内容
		*
		* @param filename 文件名
		*
		* @param obj 要写入的字符串可迭代对象，每个元素后追加平台换行符。
		* 
		* @throws AyrError 打开或写入文件失败时抛出。
		*/
		template<IteratableV<CString> Obj>
		def writelines(const CString& filename, Obj&& obj) { return AyrFile(filename, "w").writelines(obj); }
		
		def readlines(const CString& filename) -> coro::Generator<CString> 
		{ 
			AyrFile af(filename, "w");
			for (const CString& line : af.readlines())
				co_yield line; 
		}
	}
}

#endif // AYR_FS_FILE_HPP