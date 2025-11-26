#ifndef AYR_NET_UTILS_HPP
#define AYR_NET_UTILS_HPP

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../base/ExTask.hpp"
#include "../coro/IoContext.hpp"
#include "../fs/oslib.h"

namespace ayr
{
	namespace net
	{
		// 获取ssl的错误信息
		def ssl_error_msg() -> CString
		{
			Buffer buf(256);
			ERR_error_string_n(ERR_get_error(), buf.write_ptr(), buf.writeable_size());
			buf.written(std::strlen(buf.peek()));
			return from_buffer(std::move(buf));
		}

		// 是否是非阻塞模式还未就绪
		bool is_eagain()
		{
#if defined(AYR_WIN)
			int err = WSAGetLastError();
			return err == WSAEWOULDBLOCK;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
			int err = errno;
			return err == EAGAIN || err == EWOULDBLOCK;
#endif
		}

		/*
		* @brief 是否是SSL非阻塞模式还未就绪
		*
		* @param ssl SSL指针
		*
		* @param ret SSL_read或SSL_write的返回值
		*/
		bool is_ssl_eagain(SSL* ssl, int ret)
		{
			int err = SSL_get_error(ssl, ret);
			return err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE;
		}

		// 是否是正在进行连接
		bool is_einprogress()
		{
#if defined(AYR_WIN)
			int err = WSAGetLastError();
			return err == WSAEINPROGRESS;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
			int err = errno;
			return err == EINPROGRESS;
#endif
		}

		// 复制文件描述符
		int dup(int fd)
		{
#if defined(AYR_WIN)
			WSAPROTOCOL_INFO info;
			// 将 socket 信息复制出来
			if (WSADuplicateSocket(fd, GetCurrentProcessId(), &info) != 0)
				RuntimeError(get_error_msg());

			// 创建一个新的 socket，等价于 dup
			SOCKET new_sock = WSASocket(info.iAddressFamily,
				info.iSocketType,
				info.iProtocol,
				&info,
				0,
				WSA_FLAG_OVERLAPPED);
			if (new_sock == -1)
				RuntimeError(get_error_msg());

			return new_sock;
#elif defined(AYR_LINUX) || defined(AYR_MAC)
			return ::dup(fd);
#endif
		}

		/*
		* @brief 设置socket为阻塞或非阻塞模式
		*
		* @param fd 要设置模式的文件描述符
		*
		* @param blocking 是否阻塞模式，true为阻塞模式，false为非阻塞模式
		*/
		def setblocking(int fd, bool blocking)
		{
#if defined(AYR_WIN)
			u_long mode = blocking ? 0 : 1;
			if (::ioctlsocket(fd, FIONBIO, &mode) != 0)
				RuntimeError(get_error_msg());
#elif defined(AYR_LINUX)
			int flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
				RuntimeError(get_error_msg());
			if (blocking)
				flags &= ~O_NONBLOCK;
			else
				flags |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) != 0)
				RuntimeError(get_error_msg());
#endif
		}

		/*
		* @brief 设置socket选项
		*
		* @param fd 要设置选项的文件描述符
		*
		* @param level 选项级别
		*
		* @param optname 选项名称
		*
		* @param optval 选项值
		*
		* @param optlen 选项长度
		*/
		def setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen)
		{
#if defined(AYR_WIN)
			return ::setsockopt(fd, level, optname, static_cast<const char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::setsockopt(fd, level, optname, optval, optlen);
#endif
		}

		/*
		* @brief 获取socket选项
		*
		* @param fd 要获取选项的文件描述符
		*
		* @param level 选项级别
		*
		* @param optname 选项名称
		*
		* @param optval 选项值
		*
		* @param optlen 选项长度
		*/
		def getsockopt(int fd, int level, int optname, void* optval, socklen_t* optlen)
		{
#if defined(AYR_WIN)
			return ::getsockopt(fd, level, optname, static_cast<char*>(optval), optlen);
#elif defined(AYR_LINUX)
			return ::getsockopt(fd, level, optname, optval, optlen);
#endif
		}

		/*
		* @brief 设置缓冲区大小
		*
		* @param fd 要设置缓冲区的文件描述符
		*
		* @param size 缓冲区大小
		*
		* @param mode 缓冲区模式，'r'表示接收缓冲区，'w'表示发送缓冲区
		*/
		def setbuffer(int fd, int size, const CString& mode)
		{
			if (mode == "r")
				setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
			else if (mode == "w")
				setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
			else
				ValueError(std::format("Invalid buffer mode {}. Should be 'r' or 'w'.", mode));
		}

		/*
		* @brief 设置是否复用地址
		*
		* @param fd 要设置复用地址的文件描述符
		*
		* @param on 是否复用地址，true为复用地址，false为不复用地址
		*/
		def reuse_addr(int fd, bool on)
		{
			int optval = ifelse(on, 1, 0);
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		}

		def socket(int domain, int type, int protocol) -> int
		{
#if defined(AYR_WIN)
			int fd = WSASocket(domain, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
#elif defined(AYR_LINUX) || defined(AYR_MAC)
			int fd = ::socket(domain, type, protocol);
#endif
			if (fd == -1)
				RuntimeError(get_error_msg());
			return fd;
		}

		/*
		* @brief 读取socket文件描述符的数据到buffer中
		*
		* @param fd 要读取的文件描述符
		*
		* @param buffer 要读取的数据存放的buffer
		*
		* @param read_size 要读取的数据大小
		*
		* @return 实际读取的字节数, -1表示非阻塞模式下读缓冲区为空
		*/
		def read(int fd, Buffer& buffer, c_size read_size, int flags = 0)
		{
			if (read_size <= 0)
				read_size = buffer.writeable_size();
			else
				buffer.expand_util(read_size);

			int num_read = ::recv(fd, buffer.write_ptr(), read_size, flags);
			if (num_read == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_error_msg());
			buffer.written(num_read);
			return num_read;
		}

		/*
		* @brief 读取SSL指针的数据到buffer中
		*
		* @param ssl SSL指针
		*
		* @param buffer 要读取的数据存放的buffer
		*
		* @param read_size 要读取的数据大小
		*
		* @return 实际读取的字节数, -1表示非阻塞模式下读缓冲区为空
		*/
		def read(SSL* ssl, Buffer& buffer, c_size read_size)
		{
			if (read_size <= 0)
				read_size = buffer.writeable_size();
			else
				buffer.expand_util(read_size);

			int num_read = SSL_read(ssl, buffer.write_ptr(), read_size);
			if (num_read < 0)
				if (is_ssl_eagain(ssl, num_read))
					return -1;
				else
					SSLError(ssl_error_msg());

			buffer.written(num_read);
			return num_read;
		}

		/*
		* @brief 写入数据到socket文件描述符
		*
		* @param fd 要写入的文件描述符
		*
		* @param data 要写入的数据
		*
		* @return 实际写入的字节数, -1表示非阻塞模式下写缓冲区已满
		*/
		def write(int fd, const CString& data, int flags = 0) -> int
		{
			int num_written = ::send(fd, data.data(), data.size(), flags);
			if (num_written == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_error_msg());

			return num_written;
		}

		/*
		* @brief 写入数据到SSL指针
		*
		* @param ssl SSL指针
		*
		* @param data 要写入的数据
		*
		* @return 实际写入的字节数, -1表示非阻塞模式下写缓冲区已满
		*/
		def write(SSL* ssl, const CString& data) -> int
		{
			int num_written = SSL_write(ssl, data.data(), data.size());
			if (num_written < 0)
				if (is_ssl_eagain(ssl, num_written))
					return -1;
				else
					SSLError(ssl_error_msg());
			return num_written;
		}

		/*
		* @brief 将buffer中的数据写入socket文件描述符
		*
		* @param fd 要写入的文件描述符
		*
		* @param buffer 要写入的数据存放的buffer, buffer中的数据会根据已写数据大小调整
		*
		* @return 实际写入的字节数, -1表示非阻塞模式下写缓冲区已满
		*/
		def write(int fd, Buffer& buffer, int flags = 0) -> int
		{
			int num_written = ::send(fd, buffer.peek(), buffer.readable_size(), flags);
			if (num_written == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_error_msg());
			buffer.retrieve(num_written);
			return num_written;
		}

		def write(SSL* ssl, Buffer& buffer) -> int
		{
			int num_written = SSL_write(ssl, buffer.peek(), buffer.readable_size());
			if (num_written < 0)
				if (is_ssl_eagain(ssl, num_written))
					return -1;
				else
					SSLError(ssl_error_msg());
			buffer.retrieve(num_written);
			return num_written;
		}

		/*
		* @brief 与服务器建立连接
		*
		* @param fd 要等待可读的文件描述符
		*
		* @param addr 服务器地址
		*
		* @param len 服务器地址长度
		*
		* @param io_context 协程上下文
		*/
		def co_connect(int fd, const sockaddr* addr, socklen_t len, coro::IoContext* io_context) -> coro::Task<bool>
		{
			int ret = ::connect(fd, addr, len);
			if (ret == 0) co_return true;
			if (ret == -1 && !is_einprogress())
				RuntimeError(get_error_msg());
			co_await io_context->wait_for_write(fd);
			
			int result = 0;
			socklen_t result_len = sizeof(result);
			if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0)
				co_return false;

			co_return result == 0;
		}

		// 关闭socket文件描述符
		def close(int fd)
		{
#if defined(AYR_WIN)
			::closesocket(fd);
#elif defined(AYR_LINUX)
			::close(fd);
#endif
		}
	}
}
#endif // AYR_NET_UTILS_HPP