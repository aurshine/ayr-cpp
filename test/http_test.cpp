#include <ayr/net/http.hpp>

using namespace ayr;

int main()
{
	RequestParser parser;

	Socket http_fd = tcpv4();
	http_fd.bind("127.0.0.1", 7070);
	http_fd.listen();

	auto client_fd = http_fd.accept();

	while (true)
	{
		Atring req_str(client_fd.recv(1024));
		auto req = parser(req_str);

		print(req.text());

		CString msg = "hello world";
		HttpResponse response("HTTP/1.1", 200, "OK");
		response.add_header("Content-Type", "text/plain");
		response.set_body(msg);

		client_fd.send(response.text());
	}

	client_fd.close();
	http_fd.close();
	return 0;
}