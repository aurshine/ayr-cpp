#include <cerrno>

#include <openssl/err.h>

#include <ayr/net/Selector/EventContext.hpp>
#include <ayr/coro/EventAwaiter.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	UTEST_SCOPE("测试 OpenSSL 错误名称和空错误队列消息。")
	{
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_NONE), "SSL_ERROR_NONE");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_ZERO_RETURN), "SSL_ERROR_ZERO_RETURN");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_WANT_READ), "SSL_ERROR_WANT_READ");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_WANT_WRITE), "SSL_ERROR_WANT_WRITE");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_WANT_CONNECT), "SSL_ERROR_WANT_CONNECT");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_WANT_ACCEPT), "SSL_ERROR_WANT_ACCEPT");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_WANT_X509_LOOKUP), "SSL_ERROR_WANT_X509_LOOKUP");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_SYSCALL), "SSL_ERROR_SYSCALL");
		UTEST_EXPECT_EQ(net::ssl_error_name(SSL_ERROR_SSL), "SSL_ERROR_SSL");
		UTEST_EXPECT_EQ(net::ssl_error_name(-123), "SSL_ERROR_UNKNOWN");
		ERR_clear_error();
		UTEST_EXPECT_EQ(net::ssl_error_msg(), "OpenSSL error queue is empty");
		UTEST_EXPECT(net::ssl_error_msg(SSL_ERROR_SYSCALL, 0).contains("unexpectedly"));
	};

	UTEST_SCOPE("测试 errno 状态判断。")
	{
	#if defined(AYR_WIN)
		WSASetLastError(WSAEWOULDBLOCK);
		UTEST_EXPECT(net::is_eagain());
		WSASetLastError(WSAEINVAL);
		UTEST_EXPECT(!net::is_eagain());
		WSASetLastError(WSAEINPROGRESS);
		UTEST_EXPECT(net::is_einprogress());
		WSASetLastError(WSAEINVAL);
		UTEST_EXPECT(!net::is_einprogress());
	#else
		errno = EAGAIN;
		UTEST_EXPECT(net::is_eagain());
		errno = EINVAL;
		UTEST_EXPECT(!net::is_eagain());
		errno = EINPROGRESS;
		UTEST_EXPECT(net::is_einprogress());
		errno = EINVAL;
		UTEST_EXPECT(!net::is_einprogress());
	#endif
	};

	UTEST_SCOPE("测试底层 socket 创建、阻塞模式、选项和非法缓冲模式。")
	{
		BaseSocket socket = net::socket(AF_INET, SOCK_STREAM, 0);
		UTEST_EXPECT(socket != -1);
		UTEST_EXPR(net::setblocking(socket, false));
		UTEST_EXPR(net::setblocking(socket, true));
		UTEST_EXPR(net::reuse_addr(socket, true));
		int reuse = 0;
		socklen_t reuse_size = sizeof(reuse);
		UTEST_EXPECT_EQ(net::getsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, &reuse_size), 0);
		UTEST_EXPECT(reuse != 0);
		UTEST_EXPR(net::setbuffer(socket, 8192, "r"));
		UTEST_EXPR(net::setbuffer(socket, 8192, "w"));
		UTEST_EXPECT_AYR_ERROR(net::setbuffer(socket, 8192, "invalid"));
		net::close(socket);
		UTEST_EXPECT_AYR_ERROR(net::socket(-1, SOCK_STREAM, 0));
	};

	UTEST_SCOPE("测试 IoResult 默认值、工厂、复制和赋值。")
	{
		net::IoResult empty;
		UTEST_EXPECT(empty.ok());
		UTEST_EXPECT_EQ(empty.bytes, 0);
		UTEST_EXPECT_EQ(empty.socket, -1);
		auto failed = net::io_result("failed", 3, 9);
		UTEST_EXPECT(!failed.ok());
		UTEST_EXPECT_EQ(failed.error, "failed");
		UTEST_EXPECT_EQ(failed.bytes, 3);
		UTEST_EXPECT_EQ(failed.socket, 9);
		net::IoResult copied(failed);
		copied = copied;
		UTEST_EXPECT_EQ(copied.error, "failed");
	};

	UTEST_SCOPE("测试 EventContext 各事件工厂、访问器、结果写入和类型校验。")
	{
		Buffer buffer;
		net::IoResult result;
		auto read = net::EventContext::create_read_context(3, nullptr, &buffer, &result);
		UTEST_EXPECT(read.event() == net::EventOperation::READ);
		UTEST_EXPECT_EQ(read.socket(), 3);
		UTEST_EXPECT(read.coroutine() == nullptr);
		UTEST_EXPECT(read.buffer() == &buffer);
		UTEST_EXPECT(read.result() == &result);
		read.result(5, "problem");
		UTEST_EXPECT_EQ(result.bytes, 5);
		UTEST_EXPECT_EQ(result.error, "problem");
		UTEST_EXPECT_AYR_ERROR(read.remote_addrinfo());
		UTEST_EXPECT_AYR_ERROR(read.accept_socket());

		net::IoResult write_result;
		auto write = net::EventContext::create_write_context(4, nullptr, &buffer, &write_result);
		UTEST_EXPECT(write.event() == net::EventOperation::WRITE);
		UTEST_EXPECT(write.buffer() == &buffer);

		BaseSocket accepted = 11;
		net::IoResult accept_result;
		auto accept = net::EventContext::create_accept_context(5, nullptr, AF_INET, &accepted, &accept_result);
		UTEST_EXPECT(accept.event() == net::EventOperation::ACCEPT);
	#if defined(AYR_WIN)
		UTEST_EXPECT(accept.accept_socket() != -1);
		net::close(accept.accept_socket());
	#else
		UTEST_EXPECT_EQ(accept.accept_socket(), 11);
	#endif
		UTEST_EXPECT(accept.accept_socket_ptr() == &accepted);
		UTEST_EXPECT_AYR_ERROR(accept.buffer());

		addrinfo remote{};
		net::IoResult connect_result;
		auto connect = net::EventContext::create_connect_context(6, nullptr, &remote, &connect_result);
		UTEST_EXPECT(connect.event() == net::EventOperation::CONNECT);
		UTEST_EXPECT(connect.remote_addrinfo() == &remote);
		UTEST_EXPECT_AYR_ERROR(connect.buffer());

		net::EventContext copied(connect);
		net::EventContext moved(std::move(copied));
		UTEST_EXPECT(copied.event() == net::EventOperation::NONE);
		UTEST_EXPECT(moved.remote_addrinfo() == &remote);
		net::EventContext assigned;
		assigned = std::move(moved);
		assigned = std::move(assigned);
		UTEST_EXPECT(assigned.remote_addrinfo() == &remote);
	};

	return UTEST_COMPLETE();
}
