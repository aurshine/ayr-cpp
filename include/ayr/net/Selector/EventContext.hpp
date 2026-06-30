#ifndef AYR_NET_SELECTOR_EVENTCONTEXT_HPP
#define AYR_NET_SELECTOR_EVENTCONTEXT_HPP

#include <WinSock2.h>

#include "../../coro/co_utils.hpp"

namespace ayr
{
	namespace net
	{
		enum class EventOperation
		{
			NONE,
			READ,
			WRITE,
			ACCEPT,
			CONNECT
		};

		/*
		* @brief EventContext类，保存提交的事件和context信息
		*/
		class EventContext
		{
			using self = EventContext;

			// 提交的事件
			EventOperation post_event_;

			// 事件读写的字节数
			int bytes_;

			// 事件对应的socket
			BaseSocket event_socket_;

			// 事件对应的协程
			coro::Coroutine coro_;

			// Buffer*, addrinfo*, BaseSocket
			union {
				Buffer* buffer_;
				addrinfo* remote_addrinfo_;
				BaseSocket accept_socket_;
			} data_;

			EventContext(const EventOperation& op, BaseSocket socket, coro::Coroutine coro) : post_event_(op), bytes_(0), event_socket_(socket), coro_(coro), data_() {}
		public:
			EventContext(const self& other) : post_event_(other.post_event_), bytes_(other.bytes_), event_socket_(other.event_socket_), coro_(other.coro_), data_(other.data_) {}

			EventContext(self&& other) noexcept: post_event_(other.post_event_), bytes_(other.bytes_), event_socket_(other.event_socket_), coro_(other.coro_), data_(other.data_)
			{
				other.post_event_ = EventOperation::NONE;
				other.bytes_ = 0;
				other.event_socket_ = -1;
				other.coro_ = nullptr;
			}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				post_event_ = other.post_event_;
				bytes_ = other.bytes_;
				event_socket_ = other.event_socket_;
				coro_ = other.coro_;
				data_ = other.data_;
				return *this;
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				post_event_ = std::exchange(other.post_event_, EventOperation::NONE);
				bytes_ = std::exchange(other.bytes_, 0);
				event_socket_ = std::exchange(other.event_socket_, -1);
				coro_ = std::exchange(other.coro_, nullptr);
				data_ = other.data_;
				return *this;
			}

			/*
			* @brief 创建read事件上下文
			* 
			* @param socket 投递事件的socket，不管理socket的生命周期
			* 
			* @param coro 事件完成后要恢复的协程, 不管理协程的生命周期
			* 
			* @param buffer read数据的缓冲区, 不管理缓冲区的生命周期
			*/
			static self create_read_context(BaseSocket socket, coro::Coroutine coro, Buffer* buffer)
			{
				self item(EventOperation::READ, socket, coro);
				item.data_.buffer_ = buffer;
				return item;
			}

			/*
			* @brief 创建write事件上下文
			* 
			* @param socket 投递事件的socket，不管理socket的生命周期
			* 
			* @param coro 事件完成后要恢复的协程, 不管理协程的生命周期
			* 
			* @param buffer write数据的缓冲区, 不管理缓冲区的生命周期
			*/
			static self create_write_context(BaseSocket socket, coro::Coroutine coro, Buffer* buffer)
			{
				self item(EventOperation::WRITE, socket, coro);
				item.data_.buffer_ = buffer;
				return item;
			}

			/*
			* @brief 创建accept事件上下文
			* 
			* @param socket 投递事件的socket，不管理socket的生命周期
			* 
			* @param coro 事件完成后要恢复的协程, 不管理协程的生命周期
			* 
			* @param family socket的地址族
			*/
			static self create_accept_context(BaseSocket socket, coro::Coroutine coro, int family)
			{
				self item(EventOperation::ACCEPT, socket, coro);
				item.data_.accept_socket_ = net::socket(family, SOCK_STREAM, IPPROTO_TCP);
				return item;
			}

			/*
			* @brief 创建connect事件上下文
			* 
			* @param socket 投递事件的socket，不管理socket的生命周期
			* 
			* @param coro 事件完成后要恢复的协程, 不管理协程的生命周期
			* 
			* @param remote_addrinfo 远程地址信息
			*/
			static self create_connect_context(BaseSocket socket, coro::Coroutine coro, addrinfo* remote_addrinfo)
			{
				self item(EventOperation::CONNECT, socket, coro);
				item.data_.remote_addrinfo_ = remote_addrinfo;
				return item;
			}

			// 投递事件的socket
			BaseSocket socket() const { return event_socket_; }

			BaseSocket* socket_ptr() { return &event_socket_; }

			// 返回投递的事件
			EventOperation event() const { return post_event_; }

			// 事件完成后可读写的字节数
			int bytes() const { return bytes_; }

			int bytes(int _bytes) { return bytes_ = _bytes;  }

			// 事件完成后要恢复的协程
			coro::Coroutine coroutine() const { return coro_; }

			/*
			* @brief 当事件为READ和WRITE时才可以调用
			* 
			* @return 读写数据的buffer
			*/
			Buffer* buffer() const
			{
				if (post_event_ == EventOperation::READ || post_event_ == EventOperation::WRITE)
					return data_.buffer_;
				RuntimeError("Event not EventOperation::READ or EventOperation::WRITE");
			}
			
			/*
			* @brief 当事件为CONNECT时才可以调用
			* 
			* @return 远程地址信息
			*/
			addrinfo* remote_addrinfo() const
			{
				if (post_event_ == EventOperation::CONNECT)
					return data_.remote_addrinfo_;
				RuntimeError("Event not EventOperation::CONNECT");
			}

			/*
			* @brief 当事件为ACCEPT时才可以调用
			* 
			* @return 接受连接的socket
			*/
			BaseSocket accept_socket() const
			{
				if (post_event_ == EventOperation::ACCEPT)
					return data_.accept_socket_;
				RuntimeError("Event not EventOperation::ACCEPT");
			}
		};
	}
}
#endif // AYR_NET_SELECTOR_EVENTCONTEXT_HPP