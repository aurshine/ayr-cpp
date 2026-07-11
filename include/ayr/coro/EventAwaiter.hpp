#ifndef AYR_CORO_EVENTAWAITER_HPP
#define AYR_CORO_EVENTAWAITER_HPP

#include "Task.hpp"
#include "../net/Selector/Selector.hpp"

namespace ayr
{
	namespace coro
	{
		class ReadWaiter
		{
			using self = ReadWaiter;

			net::IoResult result_;

			BaseSocket socket_;

			Buffer* buffer_;

			net::Selector* selector_;
		public:
			ReadWaiter(BaseSocket socket, Buffer* buffer, net::Selector* selector) :
				socket_(socket),
				buffer_(buffer),
				selector_(selector) {}

			bool await_ready() const noexcept { return socket_ == -1 || selector_ == nullptr; }

			void await_suspend(Coroutine coro)
			{
				auto read_context = net::EventContext::create_read_context(socket_, coro, buffer_, &result_);
				selector_->post_event(read_context);
			}

			// 返回读取事件的完成结果
			net::IoResult await_resume() const noexcept { return result_; }
		};

		class WriteWaiter
		{
			using self = WriteWaiter;

			net::IoResult result_;

			BaseSocket socket_;

			Buffer* buffer_;

			net::Selector* selector_;
		public:
			WriteWaiter(BaseSocket socket, Buffer* buffer, net::Selector* selector) :
				socket_(socket),
				buffer_(buffer),
				selector_(selector) {}

			bool await_ready() const noexcept { return socket_ == -1 || selector_ == nullptr; }

			void await_suspend(Coroutine coro)
			{
				auto write_context = net::EventContext::create_write_context(socket_, coro, buffer_, &result_);
				selector_->post_event(write_context);
			}

			// 返回写入事件的完成结果
			net::IoResult await_resume() const noexcept { return result_; }
		};

		class AcceptWaiter
		{
			using self = AcceptWaiter;

			int family_;

			BaseSocket socket_;
			
			BaseSocket accept_socket_;

			net::IoResult result_;
			
			net::Selector* selector_;
		public:
			AcceptWaiter(BaseSocket socket, int family, net::Selector* selector) :
				family_(family),
				socket_(socket),
				selector_(selector) {}
			
			bool await_ready() const noexcept { return socket_ == -1 || selector_ == nullptr; }
			
			void await_suspend(Coroutine coro)
			{
				auto accept_context = net::EventContext::create_accept_context(socket_, coro, family_, &accept_socket_, &result_);
				selector_->post_event(accept_context);
			}

			// 返回accept事件的完成结果
			net::IoResult await_resume() const noexcept { return result_; }
		};

		class ConnectWaiter
		{
			using self = ConnectWaiter;

			BaseSocket socket_;

			addrinfo* remote_addrinfo_;

			net::IoResult result_;

			net::Selector* selector_;

		public:
			ConnectWaiter(BaseSocket socket, addrinfo* remote_addrinfo, net::Selector* selector) :
				socket_(socket),
				remote_addrinfo_(remote_addrinfo),
				selector_(selector) {}

			bool await_ready() const noexcept { return socket_ == -1 || selector_ == nullptr; }

			void await_suspend(Coroutine coro)
			{
				auto connect_context = net::EventContext::create_connect_context(socket_, coro, remote_addrinfo_, &result_);
				selector_->post_event(connect_context);
			}

			// 返回connect事件的完成结果
			net::IoResult await_resume() const noexcept { return result_; }
		};
	}
}
#endif // AYR_CORO_EVENTAWAITER_HPP
