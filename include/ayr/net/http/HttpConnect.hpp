#ifndef AYR_NET_HTTP_HTTPCONNECT_HPP
#define AYR_NET_HTTP_HTTPCONNECT_HPP

#include "../Socket.hpp"
#include "../../base/ExTask.hpp"

namespace ayr
{
	def http_protocol_connect(const CString& host, const CString& port)
	{
		addrinfo hints, * res = nullptr;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(host.c_str().c_str(), port.c_str().c_str(), &hints, &res) == 0)
		{
			ExTask exit([&res] { freeaddrinfo(res); });

			for (addrinfo* p = res; p; p = p->ai_next)
			{
				Socket sock(p->ai_family, p->ai_socktype);
				if (::connect(sock.fd(), p->ai_addr, p->ai_addrlen) == 0)
					return sock;
			}
		}

		return Socket(INVALID_SOCKET);
	}

	def http_connect(const CString& host) { return http_protocol_connect(host, "80"); }

	def https_connect(const CString& host) { return http_protocol_connect(host, "443"); }
}

#endif // AYR_NET_HTTP_HTTPCONNECT_HPP