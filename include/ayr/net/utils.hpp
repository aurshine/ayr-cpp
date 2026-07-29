#ifndef AYR_NET_UTILS_HPP
#define AYR_NET_UTILS_HPP

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../fs/oslib.h"

namespace ayr
{
	namespace net
	{
		// 获取ssl的错误信息
		def ssl_error_msg() -> CString
		{
			Buffer buf;
			unsigned long error = 0;
			while ((error = ERR_get_error()) != 0)
			{
				char error_msg[256];
				ERR_error_string_n(error, error_msg, sizeof(error_msg));
				if (buf.readable_size() > 0)
					buf << "; ";
				buf << "OpenSSL error " << error << ": ";
				// char数组直接交给通用operator<<可能匹配为数组对象格式；
				// 按实际NUL结尾长度追加，确保错误文本而不是数组地址进入日志。
				buf.append_bytes(error_msg, std::strlen(error_msg));
			}
			if (buf.readable_size() == 0)
				buf << "OpenSSL error queue is empty";
			return from_buffer(std::move(buf));
		}

		def ssl_error_name(int error) -> const char*
		{
			switch (error)
			{
			case SSL_ERROR_NONE: return "SSL_ERROR_NONE";
			case SSL_ERROR_ZERO_RETURN: return "SSL_ERROR_ZERO_RETURN";
			case SSL_ERROR_WANT_READ: return "SSL_ERROR_WANT_READ";
			case SSL_ERROR_WANT_WRITE: return "SSL_ERROR_WANT_WRITE";
			case SSL_ERROR_WANT_CONNECT: return "SSL_ERROR_WANT_CONNECT";
			case SSL_ERROR_WANT_ACCEPT: return "SSL_ERROR_WANT_ACCEPT";
			case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
			case SSL_ERROR_SYSCALL: return "SSL_ERROR_SYSCALL";
			case SSL_ERROR_SSL: return "SSL_ERROR_SSL";
			default: return "SSL_ERROR_UNKNOWN";
			}
		}

		/*
		* 同时保留SSL_get_error根据函数返回值判断出的错误类别，以及OpenSSL
		* 错误队列中的详细原因。调用前不能插入其他OpenSSL调用。
		*/
		def ssl_error_msg(int error, int return_value) -> CString
		{
			// SSL_ERROR_SYSCALL的底层错误必须在其他调用可能覆盖它之前保存。
			CString socket_error;
			if (error == SSL_ERROR_SYSCALL && return_value != 0)
				socket_error = get_socket_error_msg();

			Buffer buf;
			buf << "SSL error " << error << " (" << ssl_error_name(error) << ")";

			CString details = ssl_error_msg();
			buf << ": " << details;

			if (error == SSL_ERROR_SYSCALL && return_value == 0)
				buf << "; underlying transport closed unexpectedly (EOF)";
			else if (error == SSL_ERROR_SYSCALL)
				buf << "; underlying socket error: " << socket_error;

			return from_buffer(std::move(buf));
		}

		def ssl_error_msg(SSL* ssl, int return_value) -> CString
		{
			return ssl_error_msg(SSL_get_error(ssl, return_value), return_value);
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
		bool is_ssl_eagain(SSL* ssl, int ret, int* ssl_error = nullptr)
		{
			int err = SSL_get_error(ssl, ret);
			if (ssl_error != nullptr)
				*ssl_error = err;
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

		/*
		* @brief 设置socket为阻塞或非阻塞模式
		*
		* @param fd 要设置模式的文件描述符
		*
		* @param blocking 是否阻塞模式，true为阻塞模式，false为非阻塞模式
		*/
		def setblocking(BaseSocket fd, bool blocking)
		{
#if defined(AYR_WIN)
			u_long mode = blocking ? 0 : 1;
			if (::ioctlsocket(fd, FIONBIO, &mode) != 0)
				RuntimeError(win_error2str(WSAGetLastError()));
#elif defined(AYR_LINUX)
			int flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
				RuntimeError(get_system_error_msg());
			if (blocking)
				flags &= ~O_NONBLOCK;
			else
				flags |= O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) != 0)
				RuntimeError(get_system_error_msg());
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
		def setsockopt(BaseSocket fd, int level, int optname, const void* optval, socklen_t optlen)
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
		def getsockopt(BaseSocket fd, int level, int optname, void* optval, socklen_t* optlen)
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
		def setbuffer(BaseSocket fd, int size, const CString& mode)
		{
			if (mode == "r")
				setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
			else if (mode == "w")
				setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
			else
				ValueError(ayr::format("Invalid buffer mode {}. Should be 'r' or 'w'.", mode));
		}

		/*
		* @brief 设置是否复用地址
		*
		* @param fd 要设置复用地址的文件描述符
		*
		* @param on 是否复用地址，true为复用地址，false为不复用地址
		*/
		def reuse_addr(BaseSocket fd, bool on)
		{
			int optval = ifelse(on, 1, 0);
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		}

		def socket(int domain, int type, int protocol) -> BaseSocket
		{
#if defined(AYR_WIN)
			BaseSocket fd = WSASocket(domain, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
#elif defined(AYR_LINUX) || defined(AYR_MAC)
			BaseSocket fd = ::socket(domain, type, protocol);
#endif
			if (fd == -1)
				RuntimeError(get_socket_error_msg());
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
		def read(BaseSocket fd, Buffer& buffer, c_size read_size, int flags = 0)
		{
			if (read_size <= 0)
				read_size = buffer.writeable_size();
			else
				buffer.adjust_util(read_size);

			int num_read = ::recv(fd, buffer.write_ptr(), read_size, flags);
			if (num_read == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_socket_error_msg());
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
				buffer.adjust_util(read_size);

			int num_read = SSL_read(ssl, buffer.write_ptr(), read_size);
			if (num_read < 0)
			{
				int error = SSL_ERROR_NONE;
				if (is_ssl_eagain(ssl, num_read, &error))
					return -1;
				else
					SSLError(ssl_error_msg(error, num_read));
			}

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
		def write(BaseSocket fd, const CString& data, int flags = 0) -> int
		{
			int num_written = ::send(fd, data.data(), data.size(), flags);
			if (num_written == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_socket_error_msg());

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
			{
				int error = SSL_ERROR_NONE;
				if (is_ssl_eagain(ssl, num_written, &error))
					return -1;
				else
					SSLError(ssl_error_msg(error, num_written));
			}
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
		def write(BaseSocket fd, Buffer& buffer, int flags = 0) -> int
		{
			int num_written = ::send(fd, buffer.peek(), buffer.readable_size(), flags);
			if (num_written == -1)
				if (is_eagain())
					return -1;
				else
					RuntimeError(get_socket_error_msg());
			buffer.retrieve(num_written);
			return num_written;
		}

		def write(SSL* ssl, Buffer& buffer) -> int
		{
			int num_written = SSL_write(ssl, buffer.peek(), buffer.readable_size());
			if (num_written < 0)
			{
				int error = SSL_ERROR_NONE;
				if (is_ssl_eagain(ssl, num_written, &error))
					return -1;
				else
					SSLError(ssl_error_msg(error, num_written));
			}
			buffer.retrieve(num_written);
			return num_written;
		}

		// 关闭socket文件描述符
		def close(BaseSocket fd)
		{
#if defined(AYR_WIN)
			::closesocket(fd);
#elif defined(AYR_LINUX)
			::close(fd);
#endif
		}

#if defined(AYR_WIN)
		// 用于初始化Winsock的类
		class _StartSocket
		{
			using self = _StartSocket;

			// 扩展函数指针（因为它们是动态加载的）
			LPFN_ACCEPTEX lpfnAcceptEx4 = nullptr;
			
			LPFN_ACCEPTEX lpfnAcceptEx6 = nullptr;
			
			LPFN_CONNECTEX lpfnConnectEx4 = nullptr;
			
			LPFN_CONNECTEX lpfnConnectEx6 = nullptr;

			_StartSocket()
			{
				WSADATA wsaData;
				int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (error != 0)
					RuntimeError(win_error2str(error));
			}

			_StartSocket(const self&) = delete;

			_StartSocket(self&&) = delete;
		public:
			~_StartSocket() { WSACleanup(); }

			static self& singleton()
			{
				static self instance;
				return instance;
			}

			LPFN_ACCEPTEX acceptex4()
			{
				if (lpfnAcceptEx4) return lpfnAcceptEx4;

				DWORD bytes = 0;
				GUID guidAcceptEx = WSAID_ACCEPTEX;

				// 加载 AcceptEx4
				if (WSAIoctl(net::socket(AF_INET, SOCK_STREAM, 0), SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx),
					&lpfnAcceptEx4, sizeof(lpfnAcceptEx4), &bytes, NULL, NULL) == SOCKET_ERROR)
				{
					RuntimeError(ayr::format("Failed to load AcceptEx for IPv4: {}", win_error2str(WSAGetLastError())));
				}

				return lpfnAcceptEx4;
			}

			LPFN_ACCEPTEX acceptex6()
			{
				if (lpfnAcceptEx6) return lpfnAcceptEx6;

				DWORD bytes = 0;
				GUID guidAcceptEx = WSAID_ACCEPTEX;

				// 加载 AcceptEx6
				if (WSAIoctl(net::socket(AF_INET6, SOCK_STREAM, 0), SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx),
					&lpfnAcceptEx6, sizeof(lpfnAcceptEx6), &bytes, NULL, NULL) == SOCKET_ERROR)
				{
					RuntimeError(ayr::format("Failed to load AcceptEx for IPv6: {}", win_error2str(WSAGetLastError())));
				}

				return lpfnAcceptEx6;
			}

			LPFN_CONNECTEX connectex4()
			{
				if (lpfnConnectEx4) return lpfnConnectEx4;
				DWORD bytes = 0;
				GUID guidConnectEx = WSAID_CONNECTEX;
				// 加载 ConnectEx4
				if (WSAIoctl(net::socket(AF_INET, SOCK_STREAM, 0), SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
					&lpfnConnectEx4, sizeof(lpfnConnectEx4), &bytes, NULL, NULL) == SOCKET_ERROR)
				{
					RuntimeError(ayr::format("Failed to load ConnectEx for IPv4: {}", win_error2str(WSAGetLastError())));
				}
				return lpfnConnectEx4;
			}


			LPFN_CONNECTEX connectex6()
			{
				if (lpfnConnectEx6) return lpfnConnectEx6;
				DWORD bytes = 0;
				GUID guidConnectEx = WSAID_CONNECTEX;
				// 加载 ConnectEx6
				if (WSAIoctl(net::socket(AF_INET6, SOCK_STREAM, 0), SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
					&lpfnConnectEx6, sizeof(lpfnConnectEx6), &bytes, NULL, NULL) == SOCKET_ERROR)
				{
					RuntimeError(ayr::format("Failed to load ConnectEx for IPv6: {}", win_error2str(WSAGetLastError())));
				}
				return lpfnConnectEx6;
			}
		};

		// 生成一个静态实例，确保在程序启动时初始化Winsock
		static const _StartSocket& __startsocket = _StartSocket::singleton();

		BOOL acceptex(SOCKET sListenSocket,
			SOCKET sAcceptSocket,
			PVOID lpOutputBuffer,
			DWORD dwReceiveDataLength,
			DWORD dwLocalAddressLength,
			DWORD dwRemoteAddressLength,
			LPDWORD lpdwBytesReceived,
			LPOVERLAPPED lpOverlapped)
		{
			sockaddr_storage addr{};
			int len = sizeof(addr);

			::getsockname(sListenSocket, (sockaddr*)&addr, &len);
			int famliy = addr.ss_family;

			switch (famliy)
			{
			case AF_INET:
				return _StartSocket::singleton().acceptex4()(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
			case AF_INET6:
				return _StartSocket::singleton().acceptex6()(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
			default:
				RuntimeError(ayr::format("Unsupported family: {}", famliy));
				return None;
			}
		}

		BOOL connectex(SOCKET sConnectSocket,
			const struct sockaddr* name,
			int namelen,
			PVOID lpSendBuffer,
			DWORD dwSendDataLength,
			LPDWORD lpdwBytesSent,
			LPOVERLAPPED lpOverlapped)
		{
			sockaddr_storage addr{};
			int len = sizeof(addr);

			::getsockname(sConnectSocket, (sockaddr*)&addr, &len);
			int famliy = addr.ss_family;

			switch (famliy)
			{
			case AF_INET:
				return _StartSocket::singleton().connectex4()(sConnectSocket, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
			case AF_INET6:
				return _StartSocket::singleton().connectex6()(sConnectSocket, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
			default:
				RuntimeError(ayr::format("Unsupported family: {}", famliy));
				return None;
			}
		}
#endif // AYR_WIN
	}
}
#endif // AYR_NET_UTILS_HPP
