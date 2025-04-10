#include "ayr/net.hpp"

using namespace ayr;

class ChatServer : public UltraTcpServer<ChatServer>
{
	DynArray<Socket> clients;
public:
	ChatServer(const CString& ip, int port) : UltraTcpServer<ChatServer>(ip, port) {}

	void on_connected(const Socket& client)
	{
		print("a new client connected: ", client);
		clients.append(client);
	}

	void on_reading(const Socket& client)
	{
		CString message = client.recv(1024);
		print("received message from ", client, ": ", message);
		if (message == "admin  quit")
		{
			print("admin quit the server");
			stop();
			print("server stopped");
			return;
		}
		else if (message.empty())
		{
			print("disconnected signal");
			client.send("bye");
			return;
		}

		for (const Socket& c : clients)
			if (c != client)
			{
				print("sending message to ", c);
				c.sendall(message);
			}
				
	}

	void on_disconnected(const Socket& client)
	{
		print("client ", client, " disconnected");
		clients.pop_if([&client](const Socket& s) { return s == client; });
		print(clients);
	}

	// 写事件产生时的回调函数, 用于发送数据
	void on_writing(const Socket& client) {}
};

int main()
{
	ChatServer server("127.0.0.1", 5555);
	server.run();
	return 0;
}