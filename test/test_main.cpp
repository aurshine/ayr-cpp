#include "dynarray_test.hpp"
#include "socket_test.hpp"
#include "atring_test.hpp"
#include "json_test.hpp"
#include "dict_test.hpp"
#include "generator_test.hpp"
#include "when_all_test.hpp"
#include "chain_test.hpp"

using namespace ayr;

int main()
{
	MiniTcpServer server("127.0.0.1", 11451);
	server.set_recv_callback([](MiniTcpServer* server, const Socket& client) {
		CString data = client.recvmsg();

		if (!data[0])
			server->push_disconnected(client);
		else
			client.sendmsg(data);
		});

	server.set_accept_callback([](MiniTcpServer* server, const Socket& client) {
		print("client connect", client);
		});

	server.set_disconnect_callback([](MiniTcpServer* server, const Socket& client) {
		print("client disconnect", client);
		});

	server.run();

	return 0;
}