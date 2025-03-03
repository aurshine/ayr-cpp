#include <ayr/net.hpp>

using namespace ayr;

int main()
{
	Socket server_fd(AF_INET, SOCK_STREAM);
	server_fd.bind("127.0.0.1", 8080);
	server_fd.listen();

	print("HTTP server listen on 127.0.0.1:8080");

	for (int i = 0; i < 2; ++ i)
	{
		Socket client = server_fd.accept();
		print("receive request: ", client.recv(1024));

		CString response = "HTTP/1.1 200 OK\n"
			"Content-Type: text/html; charset=UTF-8\n"
			"Content-Length: 128\n"
			"\n"
			"<h1>Hello, world!</h1>";

		client.send(response);
		client.close();
	}

	server_fd.close();
	return 0;
}