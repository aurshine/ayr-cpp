#ifndef AYR_NET_SERVERCALLBACK_HPP
#define AYR_NET_SERVERCALLBACK_HPP

#include <functional>

#include "Socket.hpp"

namespace ayr
{
	template<typename Server>
	struct ServerCallback : public Object<ServerCallback<Server>>
	{
		void set_accept_callback(const std::function<void(Server*, const Socket&)>& accept_callback)
		{
			accept_callback_ = accept_callback;
		}

		void set_recv_callback(const std::function<void(Server*, const Socket&)>& recv_callback)
		{
			recv_callback_ = recv_callback;
		}

		void set_disconnect_callback(const std::function<void(Server*, const Socket&)>& disconnect_callback)
		{
			disconnect_callback_ = disconnect_callback;
		}

		void set_error_callback(const std::function<void(Server*, const Socket&, const CString&)>& error_callback)
		{
			error_callback_ = error_callback;
		}

		void set_timeout_callback(const std::function<void(Server*)>& timeout_callback)
		{
			timeout_callback_ = timeout_callback;
		}

		// 服务端接受连接后的回调函数
		// 第一个参数为当前对象，第二个参数为新连接的Socket对象
		std::function<void(Server*, const Socket&)> accept_callback_;

		// 接收到数据后的回调函数
		// 第一个参数为当前对象，第二个参数为发送数据的Socket对象
		std::function<void(Server*, const Socket&)> recv_callback_;

		// 断开连接前的回调函数
		// 第一个参数为当前对象，第二个参数为断开连接的Socket对象
		std::function<void(Server*, const Socket&)> disconnect_callback_;

		// 发生错误后的回调函数
		// 第一个参数为当前对象，第二个参数为发生错误的Socket对象, 第三参数为错误信息
		std::function<void(Server*, const Socket&, const CString&)> error_callback_;

		// 超时后的回调函数
		// 第一个参数为当前对象
		std::function<void(Server*)> timeout_callback_;
	};
}
#endif // AYR_NET_SERVERCALLBACK_HPP