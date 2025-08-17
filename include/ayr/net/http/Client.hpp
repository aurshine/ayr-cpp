#ifndef AYR_NET_HTTP_CLIENT_HPP
#define AYR_NET_HTTP_CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"

namespace ayr
{
	class Client : Object<Client>
	{
		using self = Client;

		using super = Object<Client>;

		Socket client_;

		int port_;

		CString host_;
	public:
		using Param = Dict<Atring, Atring>;

		Client(const CString& host) : host_(host) {}

		Client(const CString& host, int port) : host_(host), port_(port) {}

		HttpResponse get(const Atring& path, const Param& headers = {})
		{
			client_ = host_connect(host_, cstr(port_));
			HttpRequest req("GET", path, "HTTP/1.1", headers, {});

			req.headers.setdefault("Connection", "close");
			req.headers.setdefault("Host", host_);
			req.headers.setdefault("Accept", "*/*");
			req.headers.setdefault("Accept-Encoding", "zstd");
			req.headers.setdefault("Content-Type", "text/plain");

			client_.sendall(cstr(req.text()));
			// ResponseParser res;
		}
	};
}
#endif // AYR_NET_HTTP_CLIENT_HPP