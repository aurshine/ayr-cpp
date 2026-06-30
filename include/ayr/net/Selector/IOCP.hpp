#ifndef AYR_NET_SELECTOR_IOCP_HPP
#define AYR_NET_SELECTOR_IOCP_HPP

#include "IOCP_utils.hpp"
#include "../../air/Dict.hpp"

namespace ayr
{
	namespace net
	{
		// IOCP类，用于管理完成端口和事件的注册、投递和处理
		class IOCP
		{
			using self = IOCP;

			// 提交还没完成的事件数量
			int num_post_;

			// IOCP的完成端口句柄
			HANDLE iocp_handle_;
		public:
			IOCP(): iocp_handle_(nullptr), num_post_(0)
			{
				iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
				if (iocp_handle_ == nullptr)
					RuntimeError(ayr::format("Failed to create IOCP: {}", get_error_msg()));
			}

			IOCP(const self&) = delete;

			IOCP(self&& other) noexcept : iocp_handle_(other.iocp_handle_), num_post_(other.num_post_)
			{
				other.iocp_handle_ = nullptr;
				other.num_post_ = 0;
			}

			~IOCP() 
			{
				if (iocp_handle_ != nullptr)
					CloseHandle(iocp_handle_);
			}

			self& operator=(const self&) = delete;

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				iocp_handle_ = std::exchange(other.iocp_handle_, nullptr);
				num_post_ = std::exchange(other.num_post_, 0);
				return *this;
			}

			// 监听的fd数量
			c_size size() const { return num_post_; }

			// 是否为空
			bool empty() const { return size() == 0; }

			// 投递一个事件到iocp中
			void post_event(EventContext& ctx)
			{
				switch (ctx.event())
				{
				case EventOperation::READ:
					post_read(ctx);
					return;
				case EventOperation::WRITE:
					post_write(ctx);
					return;
				case EventOperation::ACCEPT:
					post_accept(ctx);
					return;
				case EventOperation::CONNECT:
					post_connect(ctx);
					return;
				}
			}

			// 移除并关闭fd
			void close(BaseSocket fd) { net::close(fd); }

			/*
			* @brief 设置超时，等待事件发送
			*
			* @param timeout_ms 超时时间，单位毫秒
			*
			* @return Array<EventContext> 发生的事件上下文
			*/
			Array<EventContext> wait(int timeout_ms)
			{
				OVERLAPPED_ENTRY entries[128];
				
				ULONG n = 0;
				int ret = GetQueuedCompletionStatusEx(iocp_handle_, entries, 128, &n, ifelse(timeout_ms != -1, timeout_ms, INFINITE), FALSE);
				
				if (!ret && GetLastError() != WAIT_TIMEOUT)
					RuntimeError(get_error_msg());

				Array<EventContext> results(n);
				for (int i = 0; i < n; ++i)
				{
					IOCP_OVERLAPPED* overlapped_ptr = reinterpret_cast<IOCP_OVERLAPPED*>(entries[i].lpOverlapped);
					results[i] = overlapped_ptr->context();
					results[i].bytes(entries[i].dwNumberOfBytesTransferred);
					
					// 完成accept和connect事件后，需要调用setsockopt更新socket的上下文
					if (results[i].event() == EventOperation::ACCEPT)
						complete_accept(results[i].socket(), results[i].accept_socket());
					else if (results[i].event() == EventOperation::CONNECT)
						complete_connect(results[i].socket());

					// 释放IOCP_OVERLAPPED对象的内存
					ayr_desloc(overlapped_ptr);
				}

				return results;
			}

			/*
			* @brief 等待epoll事件直到到达指定时间
			*
			* @details 超时时间为距离time_point的毫秒数，如果已经超时则立刻返回
			*
			* @param time_point 超时时间点
			*
			* @return 发生的事件列表
			*/
			Array<EventContext> wait_until(std::chrono::steady_clock::time_point time_point)
			{
				int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::steady_clock::now()).count();
				return wait(std::max(timeout_ms, 0));
			}
		private:
			// 添加completion key到iocp中， 如果已经存在则不添加
			void add_completion_key(BaseSocket* fd)
			{
				if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(fd), iocp_handle_, (ULONG_PTR)fd, 0))
					RuntimeError(ayr::format("Failed to associate socket with IOCP: {}", get_error_msg()));
			}

			// 完成accept后，更新accept_fd的accept上下文
			void complete_accept(BaseSocket listen_fd, BaseSocket accept_fd)
			{
				net::setsockopt(accept_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, &listen_fd, sizeof(listen_fd));
			}

			// 完成connect后，更新connect_fd的connect上下文
			void complete_connect(BaseSocket connect_fd)
			{
				net::setsockopt(connect_fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
			}

			/*
			* @brief 投递read事件到iocp中
			*
			* @param ctx 事件上下文
			*/
			void post_read(EventContext& ctx)
			{
				// 将socket添加到完成端口中
				add_completion_key(ctx.socket_ptr());

				DWORD bytes_received = ctx.buffer()->writeable_size();
				// 创建一个IOCP_OVERLAPPED对象
				auto overlapped_ptr = ayr_make<IOCP_OVERLAPPED>(ctx);
				// 创建一个IOCP_OVERLAPPED_BUF_DATA对象，保存缓冲区信息
				auto& overlapped_data = overlapped_ptr->emplace_data(IOCP_OVERLAPPED_BUF_DATA());
				overlapped_data.wsa_buf.buf = ctx.buffer()->write_ptr();
				overlapped_data.wsa_buf.len = ctx.buffer()->writeable_size();

				// 投递读请求到IOCP
				int ret = WSARecv(ctx.socket(), &overlapped_data.wsa_buf, 1, &bytes_received, nullptr, overlapped_ptr->overlapped_address(), nullptr);
				// 检查投递事件的返回值，如果失败则抛出异常
				if (ret == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
				{
					ayr_desloc(overlapped_ptr);
					RuntimeError(get_error_msg());
				}
			}

			/*
			* @brief 投递write事件到iocp中
			*
			* @param ctx 事件上下文
			*/
			void post_write(EventContext& ctx)
			{
				// 将socket添加到完成端口中
				add_completion_key(ctx.socket_ptr());

				DWORD bytes_sent = ctx.buffer()->readable_size();
				// 创建一个IOCP_OVERLAPPED对象
				auto overlapped_ptr = ayr_make<IOCP_OVERLAPPED>(ctx);
				// 创建一个IOCP_OVERLAPPED_BUF_DATA对象，保存缓冲区信息
				auto& overlapped_data = overlapped_ptr->emplace_data(IOCP_OVERLAPPED_BUF_DATA());
				overlapped_data.wsa_buf.buf = const_cast<char*>(ctx.buffer()->peek());
				overlapped_data.wsa_buf.len = ctx.buffer()->readable_size();

				// 投递读请求到IOCP
				int ret = WSARecv(ctx.socket(), &overlapped_data.wsa_buf, 1, &bytes_sent, nullptr, overlapped_ptr->overlapped_address(), nullptr);
				// 检查投递事件的返回值，如果失败则抛出异常
				if (ret == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
				{
					ayr_desloc(overlapped_ptr);
					RuntimeError(get_error_msg());
				}
			}

			/*
			* @brief 投递accept事件到iocp中
			*
			* @param ctx 事件上下文
			*/
			void post_accept(EventContext& ctx)
			{
				constexpr int ADDR_SIZE = sizeof(sockaddr_in) + 16;
				// 将socket添加到完成端口中
				add_completion_key(ctx.socket_ptr());

				DWORD bytes = 0;
				// 创建一个IOCP_OVERLAPPED对象
				auto overlapped_ptr = ayr_make<IOCP_OVERLAPPED>(ctx);
				// 创建一个IOCP_OVERLAPPED_ACCEPT_DATA对象，保存accept信息
				auto& overlapped_data = overlapped_ptr->emplace_data(IOCP_OVERLAPPED_ACCEPT_DATA());
				// 投递读请求到IOCP
				int ret = acceptex(ctx.socket(), ctx.accept_socket(), overlapped_data.addrin, 0, ADDR_SIZE, ADDR_SIZE, &bytes, overlapped_ptr->overlapped_address());
				// 检查投递事件的返回值，如果失败则抛出异常
				if (ret == 0 && GetLastError() != WSA_IO_PENDING)
				{
					ayr_desloc(overlapped_ptr);
					RuntimeError(get_error_msg());
				}
			}

			/*
			* @brief 投递connect事件到iocp中
			*
			* @param ctx 事件上下文
			*/
			void post_connect(EventContext& ctx)
			{
				// 将socket添加到完成端口中
				add_completion_key(ctx.socket_ptr());

				// 绑定到本地地址，端口为0，表示由系统自动分配
				sockaddr_storage local_addr{};
				socklen_t len = 0;

				if (ctx.remote_addrinfo()->ai_family == AF_INET) {
					auto* addr = reinterpret_cast<sockaddr_in*>(&local_addr);
					addr->sin_family = AF_INET;
					addr->sin_addr.s_addr = htonl(INADDR_ANY);
					addr->sin_port = htons(0);
					len = sizeof(sockaddr_in);
				}
				else if (ctx.remote_addrinfo()->ai_family == AF_INET6)
				{
					auto* addr = reinterpret_cast<sockaddr_in6*>(&local_addr);
					addr->sin6_family = AF_INET6;
					addr->sin6_addr = in6addr_any;
					addr->sin6_port = htons(0);
					len = sizeof(sockaddr_in6);
				}
				else
				{
					RuntimeError("Unsupported address family");
				}

				::bind(ctx.socket(), (sockaddr*)&local_addr, len);

				DWORD bytes = 0;
				// 创建一个IOCP_OVERLAPPED对象
				auto overlapped_ptr = ayr_make<IOCP_OVERLAPPED>(ctx);
				// 创建一个IOCP_OVERLAPPED_CONNECT_DATA对象，保存connect信息
				auto& overlapped_data = overlapped_ptr->emplace_data(IOCP_OVERLAPPED_CONNECT_DATA());
				// 投递读请求到IOCP
				int ret = connectex(ctx.socket(), ctx.remote_addrinfo()->ai_addr, sizeof(ctx.remote_addrinfo()), nullptr, 0, &bytes, overlapped_ptr->overlapped_address());
				// 检查投递事件的返回值，如果失败则抛出异常
				if (ret == 0 && GetLastError() != WSA_IO_PENDING)
				{
					ayr_desloc(overlapped_ptr);
					RuntimeError(get_error_msg());
				}
			}
		};
	}
}
#endif // AYR_NET_SELECTOR_IOCP_HPP