#ifndef AYR_NET_SOCKADDR_HPP
#define AYR_NET_SOCKADDR_HPP

#include "../filesystem.hpp"
#include "../base/raise_error.hpp"

namespace ayr
{
	struct SockAddrIn : public Object<SockAddrIn>
	{
		SockAddrIn() { memset(&addr_, 0, sizeof(sockaddr_in)); };

		SockAddrIn(const sockaddr_in& addr) : addr_(addr) {}

		SockAddrIn(const char* ip, int port, int family = AF_INET) : SockAddrIn()
		{
			addr_.sin_family = family;
			addr_.sin_port = htons(port);

			if (ip == nullptr)
				addr_.sin_addr.s_addr = INADDR_ANY;
			else if (inet_pton(AF_INET, ip, &addr_.sin_addr) != 1)
				RuntimeError(get_error_msg());
		}

		SockAddrIn(const SockAddrIn& other) : addr_(other.addr_) {}

		SockAddrIn& operator=(const SockAddrIn& other)
		{
			if (this == &other) return *this;
			return *ayr_construct(this, other);
		}

		sockaddr* get_sockaddr() { return reinterpret_cast<sockaddr*>(&addr_); }

		const sockaddr* get_sockaddr() const { return reinterpret_cast<const sockaddr*>(&addr_); }

		int get_socklen() const { return sizeof(sockaddr_in); }

		CString get_ip(int family = AF_INET) const
		{
			CString ip{ 16 };
			if (inet_ntop(family, &addr_.sin_addr, ip.data(), 16) == nullptr)
				RuntimeError(get_error_msg());
			return ip;
		}

		int get_port() const { return ntohs(addr_.sin_port); }

		CString __str__() const { return std::format("{}:{}", get_ip(), get_port()); }
	private:
		sockaddr_in addr_;
	};
}
#endif // AYR_NET_SOCKADDR_HPP