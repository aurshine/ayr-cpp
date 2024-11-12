#pragma once

#include <thread>

#include <ayr/socket/tcp.hpp>
#include <ayr/socket/udp.hpp>


using namespace std::chrono_literals;

constexpr char IP[] = "";
constexpr int PORT = 14514;

void tcp_echo_server_test()
{
	ayr::TcpServer server(nullptr, PORT);
	Socket& client = server.accept();
	while (true)
	{
		CString data = server.recv(0);
		if (!data) break;
		server.send(0, data, data.size());
	}
	print("server exit");
}

void tcp_echo_client_test()
{
	ayr::TcpClient client(IP, PORT);

	while (true)
	{
		char data[1024];
		print.setend(" ");
		print("input:");
		print.setend("\n");
		std::cin >> data;
		if (data[0] == 'q' || data[0] == 'Q')
			break;

		client.send(data, strlen(data));
		ayr::print("server response:", client.recv());
	}
	print("client exit");
}

void tcp_echo_test()
{
	std::thread server(tcp_echo_server_test);
	std::this_thread::sleep_for(1s); // 等待服务端启动
	std::thread client(tcp_echo_client_test);

	server.join();
	client.join();
}