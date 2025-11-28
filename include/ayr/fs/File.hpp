#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include "oslib.h"
#include "../Atring.hpp"
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
			
			constexpr static const char* SYS_EOL = "\r\n";
#else
			using FD = int;

			constexpr static FD INVALID_FD = -1;

			constexpr static const char* SYS_EOL = "\n";
#endif
			FD fd_;
		public:
			/*
			* @brief AyrFile构造函数
			* 
			* @param filename 文件名
			* 
			* @param mode 打开模式，支持"w"、"r"、"a"
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
					ValueError(std::format("Invalid value {}, that only support [w, r, a]", mode));

				fd_ = CreateFileA(
					filename.c_str(),
					dwDesiredAccess,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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

				fd_ = ::open(filename.c_str(), flags, 0666);

				if (fd_ == -1)
					SystemError("Failed to create or open file");
#endif
			}

			AyrFile(const Atring& filename, const CString& mode) : AyrFile(filename.encode(), mode) {}

			constexpr AyrFile(FD fd) : fd_(fd) {}

			constexpr AyrFile(self&& other) noexcept : fd_(other.fd_) { other.fd_ = INVALID_FD; }

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;

				close();
				fd_ = other.fd_;
				other.fd_ = INVALID_FD;
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
			* @brief 将文件内容读入缓冲区
			* 
			* @param buffer 要读入的缓冲区
			* 
			* @param size 要读入数据的长度，默认为-1，表示读取整个文件
			* 
			* @return 实际读取的字节数
			*/
			c_size read(Buffer& buffer, c_size size = -1) const
			{
				size = ifelse(size > 0, size, file_size());
				buffer.expand_util(size);
				// 初始buffer里的字节数
				c_size begin_buffer_size = buffer.readable_size();
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
					read_bytes = ::read(fd_, buffer.write_ptr(), buffer.writeable_size());
					ok = read_bytes >= 0;
#endif
					if (!ok)
						SystemError(std::format("Failed to read from file, {}", get_error_msg()));
					// 读完了
					if (read_bytes == 0) break;
					buffer.written(read_bytes);
				}
				return buffer.readable_size() - begin_buffer_size;
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
				read(buffer, size);
				return buffer;
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
			* @brief 根据编码读取文件内容
			* 
			* @template C 编码类型, 默认是Codec(utf-8)
			* 
			* @return 返回编码后的字符串
			*/
			template<UniCodec C = Codec>
			Atring reads() const { return "\n"as.join(readliness<C>()); }

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
				// 实际读取的字节数
				c_size num_read = 0;

				do {
					num_read = read(buffer, BLOCK_SIZE);
					/*
					* 当需要read_buffer时，表示前befor_read_size都没有换行符
					* 
					* 第一次获取eol_pos时，从befor_read_size开始查找
					* 
					* 后续获取eol_pos时，下标0开始查找
					*/
					for (eol_pos = buffer.find_eol(before_read_size); eol_pos != -1; eol_pos = buffer.find_eol())
					{
#if defined(AYR_WIN)
						// windows的换行符是\r\n
						if (eol_pos > 1 && *(buffer.peek() + eol_pos - 2) == '\r')
							co_yield vstr(buffer.peek(), eol_pos - 1);
						else
							co_yield vstr(buffer.peek(), eol_pos);
#else
						co_yield vstr(buffer.peek(), eol_pos);
#endif
						// 先yield再retrieve, 可以使用vstr
						buffer.retrieve(eol_pos + 1);
					}
					before_read_size = buffer.readable_size();
				} while (num_read == BLOCK_SIZE);
				
				// 最后一行
				if (buffer.readable_size() > 0)
					co_return from_buffer(std::move(buffer));
			}

			/*
			* @brief 按行读取指定编码文件内容
			* 
			* @template C 编码类型, 默认是Codec(utf-8)
			*/
			template<UniCodec C = Codec>
			coro::Generator<Atring> readliness() const
			{
				for (auto& line : readlines())
					co_yield Atring::from<C>(line);
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
				if (size < 0 || size > data.size())
					size = data.size();
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
					written_bytes = ::write(fd_, data.data() + num_written, size - num_written);
					ok = written_bytes >= 0;
#endif 
					if (!ok)
						SystemError(std::format("Failed to write from file, {}", get_error_msg()));
					num_written += written_bytes;
				}
			}

			/*
			* @brief 写入指定长度数据到文件
			*
			* @param buffer 要写入的数据
			*
			* @param size 要写入数据的长度，默认为-1，表示写入整个数据
			*/
			void write(Buffer& buffer, c_size size = -1)
			{
				if (size < 0 || size > buffer.readable_size())
					size = buffer.readable_size();
				write(vstr(buffer.peek(), size), size);
				buffer.retrieve(size);
			}

			/*
			* @brief 按行写入文件内容
			* 
			* @template C 编码类型, 默认是Codec(utf-8)
			* 
			* @param data 要写入的数据
			* 
			* @param size 要写入数据的长度，默认为-1，表示写入整个数据
			*/
			template<UniCodec C = Codec>
			def writes(const Atring& data, c_size size = -1) const
			{
				if (size < 0 || size > data.size())
					size = data.size();
#if defined(AYR_WIN)
				if (!data.contains("\n"as))
					write(data.vslice(0, size).encode<C>());
				else
					write(data.vslice(0, size).replace("\n"as, "\r\n"as).encode<C>());
#else
				write(data.vslice(0, size).encode<C>());
#endif
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

			/*
			* @brief 按行写入文件指定编码内容
			* 
			* @template C 编码类型, 默认是Codec(utf-8)
			* 
			* @param obj 要写入的字符串可迭代对象
			*/
			template<IteratableV<Atring> Obj, UniCodec C = Codec>
			void writeliness(Obj&& obj) const { writes<C>(Atring::from_utf8(SYS_EOL).join(obj)); }

			// 读取文件大小
			c_size file_size() const
			{
#if defined(AYR_WIN)
				DWORD high;
				DWORD low = GetFileSize(fd_, &high);

				return (uint64_t(high) << 32) | low;
#elif defined(AYR_LINUX)
				struct stat st;
				if (::fstat(fd_, &st) == -1)
					SystemError("Failed to get file size");
				return st.st_size;
#endif
			}
		};

		/*
		* @brief 读取文件内容
		*
		* @param filename 文件名
		*
		* @return 返回读取的文件内容
		*/
		def read(const CString& filename) { return AyrFile(filename, "r").read(); }

		/*
		* @brief 读取文件所有内容
		*
		* @param filename 文件名
		*
		* @param buffer 要读入的缓冲区
		*
		* @return 返回实际读取的字节数
		*/
		def read(const CString& filename, Buffer& buffer) { return AyrFile(filename, "r").read(buffer); }

		/*
		* @brief 读取文件指定编码内容
		* 
		* @template C 编码类型, 默认是Codec(utf-8)
		* 
		* @param filename 文件名
		* 
		* @return 返回编码后的字符串
		*/
		template<UniCodec C = Codec>
		def reads(const CString& filename) { return AyrFile(filename, "r").reads<C>(); }

		/*
		* @brief 写入文件内容
		*
		* @param filename 文件名
		*
		* @param data 要写入的数据
		*
		* @param size 要写入数据的长度，默认为-1，表示写入整个数据
		*/
		def write(const CString& filename, const CString& data, c_size size = -1) { return AyrFile(filename, "w").write(data, size); }

		/*
		* @brief 写入文件内容
		*
		* @param filename 文件名
		*
		* @param buffer 要写入的数据
		*
		* @param size 要写入数据的长度，默认为-1，表示写入整个数据
		*/
		def write(const CString& filename, Buffer& buffer, c_size size = -1) { return AyrFile(filename, "w").write(buffer, size); }
		
		/*
		* @brief 写入文件指定编码内容
		* 
		* @template C 编码类型, 默认是Codec(utf-8)
		* 
		* @param filename 文件名
		* 
		* @param data 要写入的数据
		* 
		* @param size 要写入数据的长度，默认为-1，表示写入整个数据
		*/
		template<UniCodec C = Codec>
		def writes(const CString& filename, const Atring& data, c_size size = -1) { return AyrFile(filename, "w").writes<C>(data, size); }
		
		/*
		* @brief 按行写入文件内容
		* 
		* @param filename 文件名
		* 
		* @param obj 要写入的字符串可迭代对象
		*/
		template<IteratableV<CString> Obj>
		def writelines(const CString& filename, Obj&& obj) { return AyrFile(filename, "w").writelines(obj); }
		
		/*
		* @brief 按行写入文件指定编码内容
		* 
		* @template C 编码类型, 默认是Codec(utf-8)
		* 
		* @param filename 文件名
		* 
		* @param obj 要写入的字符串可迭代对象
		*/
		template<IteratableV<Atring> Obj, UniCodec C = Codec>
		def writeliness(const CString& filename, Obj&& obj) { return AyrFile(filename, "w").writeliness<C>(obj); }
	}
}

#endif // AYR_FS_FILE_HPP