#ifndef AYR_NET_SELECTOR_EPOLL_HPP
#define AYR_NET_SELECTOR_EPOLL_HPP

#include <sys/epoll.h>

#include <chrono>

#include "Data.hpp"
#include "../../../air/Dict.hpp"

namespace ayr
{
	namespace net
	{
		class Epoll
		{
			using self = Epoll;

			BaseSocket epoll_fd_;

			// 保存fd和事件的映射
			Dict<BaseSocket, EpollData> fd_events_;

			int num_post_;
		public:
			Epoll() : epoll_fd_(::epoll_create1(0)), fd_events_(), num_post_(0) {}

			Epoll(const self& other) = delete;

			Epoll(self&& other) :
				epoll_fd_(std::exchange(other.epoll_fd_, -1)),
				fd_events_(std::move(other.fd_events_)) ,
				num_post_(other.num_post_){}

			~Epoll() { net::close(epoll_fd_); }

			self& operator=(const self& other) = delete;

			self& operator=(self&& other)
			{
				if (this == &other) return *this;

				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// 监听的fd数量
			c_size size() const { return num_post_; }

			// 是否为空
			bool empty() const { return size() == 0; }

			// 投递一个事件到epoll中
			void post_event(EventContext& ctx)
			{
				switch (ctx.event())
				{
				case EventOperation::READ:
					post_event_impl(EPOLLIN, ctx);
					break;
				case EventOperation::WRITE:
					post_event_impl(EPOLLOUT, ctx);
					break;
				case EventOperation::ACCEPT:
					post_event_impl(EPOLLIN, ctx);
					break;
				case EventOperation::CONNECT:
					post_event_impl(EPOLLOUT, ctx);
					break;
				}
				++num_post_;
			}

			// 移除并关闭fd
			void close(BaseSocket fd)
			{
				::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
				fd_events_.pop(fd);
				net::close(fd);
			}

			/*
			* @brief 等待epoll事件
			*
			* @param timeout_ms 超时时间，单位毫秒
			*
			* @return 发生的事件列表
			*/
			DynArray<EventContext> wait(int timeout_ms)
			{
				Array<epoll_event> evs(size());
				int n = ::epoll_wait(epoll_fd_, evs.data(), evs.size(), timeout_ms);
				if (n == -1)
					RuntimeError(get_system_error_msg());
				
				DynArray<EventContext> results;
				for (int i = 0; i < n; ++i)
				{
					auto epoll_data = reinterpret_cast<EpollData*>(evs[i].data.ptr);

					if (epoll_data->has_read())
					{
						auto& ctx = epoll_data->read_ctx();
						switch (ctx.event())
						{
						case EventOperation::READ:
							complete_read(ctx);
							break;
						case EventOperation::ACCEPT:
							complete_accept(ctx);
							break;
						default:
							RuntimeError("Invalid event type, only READ and ACCEPT are supported.");
						}
						results.append(ctx);
					}

					if (epoll_data->has_write())
					{
						auto& ctx = epoll_data->write_ctx();
						switch (ctx.event())
						{
						case EventOperation::WRITE:
							complete_write(ctx);
							break;
						case EventOperation::CONNECT:
							complete_connect(ctx);
							break;
						default:
							RuntimeError("Invalid event type, only WRITE and CONNECT are supported.");
						}
						results.append(ctx);
					}
				}
				num_post_ -= results.size();
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
			DynArray<EventContext> wait_until(std::chrono::steady_clock::time_point time_point)
			{
				int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::steady_clock::now()).count();
				return wait(std::max(timeout_ms, 0));
			}
		private:
			/*
			* @brief 投递read事件到epoll中
			*
			* @param ctx 事件上下文
			*/
			void post_event_impl(int event, EventContext& ctx)
			{
				auto& epoll_data = fd_events_[ctx.socket()];
				struct epoll_event ev;
				int op = ifelse(event == EPOLLIN, epoll_data.add_read_event(ctx), epoll_data.add_write_event(ctx));
				epoll_data.set_epoll_event(&ev);
				if (::epoll_ctl(epoll_fd_, op, ctx.socket(), &ev) == -1)
					RuntimeError(get_system_error_msg());
			}

			// read事件产生后的完成动作
			void complete_read(EventContext& ctx)
			{
				Buffer* buffer = ctx.buffer();
				int total_read = 0;
				CString error;
				while (buffer->writeable_size())
				{
					int num_read = ::recv(ctx.socket(), buffer->write_ptr(), buffer->writeable_size(), 0);

					if (num_read <= 0)
					{
						int read_error = errno;
						// 非阻塞模式还未就绪
						if (num_read < 0 && read_error != EAGAIN && read_error != EWOULDBLOCK)
							error = c_error2str(read_error);
						// num_read == 0, 对端关闭连接
						break;
					}

					buffer->written(num_read);
					total_read += num_read;
				}

				ctx.result(total_read, std::move(error));
				fd_events_[ctx.socket()].pop_read_event();
			}

			// write事件产生后的完成动作
			void complete_write(EventContext& ctx)
			{
				Buffer* buffer = ctx.buffer();
				int total_write = 0;
				CString error;
				while (buffer->readable_size() != 0)
				{
					int num_written = ::send(ctx.socket(), buffer->peek(), buffer->readable_size(), 0);
					
					if (num_written <= 0)
					{
						int write_error = errno;
						// 非阻塞模式还未就绪
						if (num_written < 0 && write_error != EAGAIN && write_error != EWOULDBLOCK)
							error = c_error2str(write_error);
						// num_written == 0, 对端关闭连接
						break;
					}
						
					buffer->retrieve(num_written);
					total_write += num_written;
				}

				ctx.result(total_write, std::move(error));
				fd_events_[ctx.socket()].pop_write_event();
			}

			// accept事件产生后的完成事件
			void complete_accept(EventContext& ctx)
			{
				*ctx.accept_socket_ptr() = ::accept(ctx.socket(), nullptr, nullptr);
				if (ctx.accept_socket() == -1)
				{
					ctx.result(0, c_error2str(errno));
				}
				else
				{
					ctx.result(0);
					ctx.result()->socket = ctx.accept_socket();
				}
				fd_events_[ctx.socket()].pop_read_event();
			}

			// connect事件产生后的完成事件
			void complete_connect(EventContext& ctx)
			{
				addrinfo* remote_addr = ctx.remote_addrinfo();
				int connect_ret = ::connect(ctx.socket(), remote_addr->ai_addr, remote_addr->ai_addrlen);
				if (connect_ret < 0)
				{
					int connect_error = errno;
					if (connect_error != EINPROGRESS &&
						connect_error != EALREADY &&
						connect_error != EISCONN)
					{
						ctx.result(0, c_error2str(connect_error));
						fd_events_[ctx.socket()].pop_write_event();
						return;
					}
				}

				int err = 0;
				socklen_t err_len = sizeof(err);
				if (net::getsockopt(ctx.socket(), SOL_SOCKET, SO_ERROR, &err, &err_len) < 0)
					ctx.result(0, c_error2str(errno));
				else if (err != 0)
					ctx.result(0, c_error2str(err));
				else
					ctx.result(0);
				fd_events_[ctx.socket()].pop_write_event();
			}
		};
	}
}

#endif // AYR_NET_SELECTOR_EPOLL_HPP
