#ifndef AYR_NET_SELECTOR_IOCP_UTILS_HPP
#define AYR_NET_SELECTOR_IOCP_UTILS_HPP

#include <variant>

#include "EventContext.hpp"
#include "../utils.hpp"
#include "../../fs/oslib.h"

namespace ayr
{
	namespace net
	{
		// IOCP的CompletionKey类，用于保存完成端口的socket相关信息
		struct CompletionKey
		{
			BaseSocket fd;

			CompletionKey(BaseSocket f) : fd(f) {}
		};

		// IOCP的OVERLAPPED结构体的扩展数据类型，用于保存缓冲区信息
		struct IOCP_OVERLAPPED_BUF_DATA
		{
			WSABUF wsa_buf{};
		};

		// IOCP的OVERLAPPED结构体的扩展数据类型，用于保存accept信息
		struct IOCP_OVERLAPPED_ACCEPT_DATA
		{
			char addrin[(sizeof(sockaddr_in) + 16) * 2];
		};

		// IOCP的OVERLAPPED结构体的扩展数据类型，用于保存connect信息
		struct IOCP_OVERLAPPED_CONNECT_DATA
		{

		};

		// 概念，用于约束IOCP_OVERLAPPED_DATA类型
		template<typename T>
		concept IOCP_OVERLAPPED_DATA_CONCEPT = issame<T,
			IOCP_OVERLAPPED_BUF_DATA,
			IOCP_OVERLAPPED_ACCEPT_DATA,
			IOCP_OVERLAPPED_CONNECT_DATA
		>;

		// IOCP的OVERLAPPED结构体，用于保存完成端口的事件相关信息
		class IOCP_OVERLAPPED
		{
			using self = IOCP_OVERLAPPED;

			OVERLAPPED ov_;

			EventContext ctx_;

			std::variant<
				IOCP_OVERLAPPED_BUF_DATA,
				IOCP_OVERLAPPED_ACCEPT_DATA,
				IOCP_OVERLAPPED_CONNECT_DATA
			> data_;
		public:
			IOCP_OVERLAPPED(const EventContext& ctx) : ov_(), ctx_(ctx), data_() {}

			IOCP_OVERLAPPED(const self&) = delete;

			IOCP_OVERLAPPED(self&& other) noexcept : ov_(std::move(other.ov_)), ctx_(std::move(other.ctx_)), data_(std::move(other.data_)) {})

			self& operator=(const self&) = delete;

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ov_ = std::move(other.ov_);
				ctx_ = std::move(other.ctx_);
				data_ = std::move(other.data_);
				return *this;
			}

			template<IOCP_OVERLAPPED_DATA_CONCEPT T>
			T& emplace_data(T&& data)
			{
				data_ = std::forward<T>(data);
				return std::get<T>(data_);
			}

			template<IOCP_OVERLAPPED_DATA_CONCEPT T>
			T& get() { return std::get<T>(data_); }

			template<IOCP_OVERLAPPED_DATA_CONCEPT T>
			const T& get() const { return std::get<T>(data_); }

			OVERLAPPED* overlapped_address() { return &ov_; }

			const EventContext& context() const { return ctx_; }

			EventContext& context() { return ctx_; }
		};
	}
}
#endif // AYR_NET_SELECTOR_IOCP_UTILS_HPP