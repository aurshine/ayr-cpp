#ifndef AYR_NET_IOWAITER_HPP
#define AYR_NET_IOWAITER_HPP

#include <utility>

#include "Socket.hpp"

namespace ayr
{
	class IoEvent : public Object<IoEvent>
	{
	public:
		using Data = View;

		using Flag = uint32_t;

		constexpr static Flag NONE = 0; 

		constexpr static Flag READABLE = 1;

		constexpr static Flag WRITABLE = 2;

		constexpr static Flag ERROR = 4;
	private:
		using self = IoEvent;

		using super = Object<IoEvent>;

		// 注册的事件和发生的事件
		Flag registered_flags_, happened_flags_;

	public:
		Data data;

		IoEvent(const Flag& flags, const Data& data) : registered_flags_(flags), happened_flags_(NONE), data(data) {}

		IoEvent(const IoEvent& other): IoEvent(other.registered_flags_, other.data) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			registered_flags_ = other.registered_flags_;
			data = other.data;
			return *this;
		}

		Flag registered_events() const { return registered_flags_; }

		void set_events(const Flag& flag) { happened_flags_ = flag; }

		Flag events() const { return happened_flags_; }
	};


	/**
	 * @brief IoWaiter类
	 *
	 * @details io多路复用对象接口，Select，Epoll的基类
	 */
	template<typename Derived>
	class IoWaiter : public Object<IoWaiter<Derived>>
	{
		using self = IoWaiter<Derived>;

		using super = Object<IoWaiter>;

		// 保存fd和事件的映射
		Dict<int, IoEvent> fd_events;
	public:

		IoWaiter() : fd_events() {}

		IoWaiter(IoWaiter&& other) noexcept : fd_events(std::move(other.fd_events)) {}

		self& operator=(self&& other) noexcept
		{
			if (this == &other) return *this;
			fd_events = std::move(other.fd_events);
			return *this;
		}

		/*
		* @brief 设置超时，等待事件发送
		*
		* @param timeout_ms 超时时间，单位毫秒
		*
		* @return Array<IoEvent> 发生的事件
		*/
		Array<IoEvent> wait(int timeout_ms) { NotImplementedError("wait not implemented"); return None; }

		/*
		* @brief 添加fd的事件
		*
		* @param fd 要设置的fd
		*
		* @param io_event 要设置的事件
		*/
		void add(int fd, const IoEvent& io_event) { fd_events.insert(fd, io_event); }

		// 监听的fd数量
		c_size size() const { return fd_events.size(); }

		// 移除fd，但不关闭fd
		int pop(int fd) { fd_events.pop(fd); return fd; }

		// 移除并关闭fd
		void close(int fd)
		{
			static_cast<Derived&>(*this).pop(fd);
#if defined(AYR_WIN)
			::closesocket(fd);
#elif defined(AYR_LINUX)
			::close(fd);
#endif
		}
	};
}
#endif // AYR_NET_IOWAITER_HPP