#ifndef AYR_NET_SELECTOR_EVENTCONTEXT_HPP
#define AYR_NET_SELECTOR_EVENTCONTEXT_HPP

#if defined(AYR_WIN)
#include <WinSock2.h>
#endif 

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
		* @brief IO事件完成结果
		*
		* @details
		* error == ERROR_SUCCESS 表示IOCP完成事件成功。
		* error != ERROR_SUCCESS 表示IOCP完成事件失败，error保存完成状态码。
		*
		* bytes:
		* - READ成功时表示读取到的字节数，bytes == 0 表示对端有序关闭连接。
		* - WRITE成功时表示写出的字节数，bytes == 0 表示本次没有写出数据。
		* - ACCEPT和CONNECT成功时通常为0。
		*
		* socket:
		* - ACCEPT成功时表示新接受的socket。
		* - READ、WRITE、CONNECT成功时表示发起该事件的socket。
		* - 失败时表示与该事件相关的socket，可能需要调用者关闭或丢弃。
		*/
		struct IoResult
		{
			int bytes = 0;

#ifdef AYR_WIN
			// Windows下使用DWORD表示错误码
			using Error_t = DWORD;
#elif defined(AYR_LINUX)
			// Linux下使用int表示错误码
			using Error_t = int;
#endif
			Error_t error = 0;

			BaseSocket socket = -1;

			bool ok() const { return error == 0; }
		};

		/*
		* @brief EventContext类，保存提交的事件和context信息
		*/
		class EventContext
		{
			using self = EventContext;

			// 提交的事件
			EventOperation post_event_;

			// 事件对应的socket
			BaseSocket event_socket_;

			// 事件对应的协程
			coro::Coroutine coro_;

			// 事件完成结果
			IoResult* result_;

			// Buffer*, addrinfo*, BaseSocket
			union {
				Buffer* buffer_;
				addrinfo* remote_addrinfo_;
				BaseSocket* accept_socket_;
			} data_;

			EventContext(const EventOperation& op, BaseSocket socket, coro::Coroutine coro, IoResult* result = nullptr) : post_event_(op), result_(result), event_socket_(socket), coro_(coro), data_() {}
		public:
			EventContext() : EventContext(EventOperation::NONE, -1, nullptr, nullptr) {}

			EventContext(const self& other) : post_event_(other.post_event_), result_(other.result_), event_socket_(other.event_socket_), coro_(other.coro_), data_(other.data_) {}

			EventContext(self&& other) noexcept: post_event_(other.post_event_), result_(other.result_), event_socket_(other.event_socket_), coro_(other.coro_), data_(other.data_)
			{
				other.post_event_ = EventOperation::NONE;
				other.result_ = nullptr;
				other.event_socket_ = -1;
				other.coro_ = nullptr;
			}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				post_event_ = other.post_event_;
				result_ = other.result_;
				event_socket_ = other.event_socket_;
				coro_ = other.coro_;
				data_ = other.data_;
				return *this;
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				post_event_ = std::exchange(other.post_event_, EventOperation::NONE);
				result_ = std::exchange(other.result_, nullptr);
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
			static self create_read_context(BaseSocket socket, coro::Coroutine coro, Buffer* buffer, IoResult* result)
			{
				self item(EventOperation::READ, socket, coro, result);
				item.data_.buffer_ = buffer;
				item.result_->socket = socket;
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
			static self create_write_context(BaseSocket socket, coro::Coroutine coro, Buffer* buffer, IoResult* result)
			{
				self item(EventOperation::WRITE, socket, coro, result);
				item.data_.buffer_ = buffer;
				item.result_->socket = socket;
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
			static self create_accept_context(BaseSocket socket, coro::Coroutine coro, int family, BaseSocket* accept_socket, IoResult* result)
			{
				self item(EventOperation::ACCEPT, socket, coro, result);
#ifdef AYR_WIN
				*accept_socket = net::socket(family, SOCK_STREAM, IPPROTO_TCP);
				item.data_.accept_socket_ = accept_socket;
				item.result_->socket = *accept_socket;
#elif defined(AYR_LINUX)
				item.data_.accept_socket_ = accept_socket;
				item.result_->socket = -1;
#endif
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
			static self create_connect_context(BaseSocket socket, coro::Coroutine coro, addrinfo* remote_addrinfo, IoResult* result)
			{
				self item(EventOperation::CONNECT, socket, coro, result);
				item.data_.remote_addrinfo_ = remote_addrinfo;
				item.result_->socket = socket;
				return item;
			}

			// 投递事件的socket
			BaseSocket socket() const { return event_socket_; }

			// 返回投递的事件
			EventOperation event() const { return post_event_; }

			// 事件完成结果
			IoResult* result() const { return result_; }

			IoResult& result(int bytes, IoResult::Error_t error)
			{
				result_->bytes = bytes;
				result_->error = error;
				return *result_;
			}

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
					return *data_.accept_socket_;
				RuntimeError("Event not EventOperation::ACCEPT");
			}

			BaseSocket* accept_socket_ptr() const
			{
				if (post_event_ == EventOperation::ACCEPT)
					return data_.accept_socket_;
				RuntimeError("Event not EventOperation::ACCEPT");
			}
		};
	}
}
#endif // AYR_NET_SELECTOR_EVENTCONTEXT_HPP