#pragma once

#include <thread>

#include <ayr/socket/tcp.hpp>
#include <ayr/socket/udp.hpp>


using namespace std::chrono_literals;

constexpr int PORT = 14514;

void tcp_echo_server_test()
{
	ayr::TcpServer server("127.0.0.1", PORT);
	Socket& client = server.accept();
	while (true)
	{
		CString data = server.recv(0);
		if (!data) break;
		server.send(0, data, data.size());
	}
	print("tcp server exit");
}

void tcp_echo_client_test()
{
	ayr::TcpClient client("127.0.0.1", PORT);

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
	print("tcp client exit");
}

void tcp_echo_test()
{
	std::thread server(tcp_echo_server_test);
	std::this_thread::sleep_for(1s); // 等待服务端启动
	std::thread client(tcp_echo_client_test);

	server.join();
	client.join();
}

void udp_echo_server_test()
{
	ayr::UdpServer server("127.0.0.1", PORT);
	while (true)
	{
		auto [data, client_addr] = server.recv();

		if (!data) break;
		server.send(data, data.size(), client_addr);
	}
	print("udp server exit");
}

void udp_echo_client_test()
{
	ayr::UdpClient client("127.0.0.1", PORT);

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
		ayr::print("server response:", client.recv().first);
	}
	print("udp client exit");
}

void udp_echo_test()
{
	std::thread server(udp_echo_server_test);
	std::this_thread::sleep_for(1s); // 等待服务端启动
	std::thread client(udp_echo_client_test);

	server.join();
	client.join();
}