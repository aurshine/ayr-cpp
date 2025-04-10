#include <ayr/net/http/Request.hpp>

using namespace ayr;

int main()
{
	RequestParser parser;

	Socket http_fd = tcpv4();
	http_fd.bind("127.0.0.1", 7070);
	http_fd.listen();

	auto client_fd = http_fd.accept();

	for (int i = 0; i < 5; i++)
	{
		Atring req_str(client_fd.recv(1024));
		auto req = parser(req_str);

		print(req.text());

		CString msg = "hello world";

		client_fd.send(std::format("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}{}", msg.size() + 1, msg, i));
	}

	client_fd.close();
	http_fd.close();
	return 0;
}