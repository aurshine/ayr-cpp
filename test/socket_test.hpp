#pragma once

#include <thread>

#include <ayr/net.hpp>


using namespace std::chrono_literals;

constexpr int PORT = 14514;

void tcp_echo_server_test()
{
	ayr::MiniTcpServer server("10.120.35.126", PORT);
	print("tcp server start at port:", PORT);
	server.set_accept_callback([](MiniTcpServer* server, const Socket& client) {
		print("new client:", client);
		});

	server.set_recv_callback([](MiniTcpServer* server, const Socket& client, const CString& data) {
		if (!data)
		{
			print("client disconnect:", client);
			server->push_disconnected(client);
			print("num clients:", server->num_clients());
		}
		else
		{
			print("recv from client:", client, "data:", data, "size:", data.size());
			client.send(data, data.size());
		}
		});

	server.set_timeout_callback([](MiniTcpServer* server) { print("timeout"); });
	server.run(5);

	print("tcp server exit");
}

void tcp_echo_client_test()
{
	ayr::Socket client(AF_INET, SOCK_STREAM);
	client.connect("127.0.0.1", PORT);

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
		print("server response:", client.recv());
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