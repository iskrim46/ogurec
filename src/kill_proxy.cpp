#include <cstdint>
#include <span>
#include <thread>

#include "net/client.hpp"
#include "net/conn.hpp"
#include "net/server.hpp"
#include "net/types.hpp"

void forward_packets(net::conn& dst, net::conn& src)
{
	for (int i = 1; i < 255; i++) {
		src.reg_handler(i, [&dst, i](std::span<uint8_t> data) {
			dst.send_packet(i, data);
			return true;
		});
	}
}

int main(int argc, char* argv[])
{
	auto server = net::client("localhost", 7777);
	auto proxy = net::server("localhost", 8888);

	proxy.bind();
	proxy.listen(1);

	auto server_conn = server.connect();
	auto proxy_conn = proxy.accept();

	uint8_t slot;
	server_conn.reg_handler<net::packet::accept>([&slot](auto& a) {
		slot = a.client_id;
		return false;
	});

	forward_packets(server_conn, proxy_conn);
	forward_packets(proxy_conn, server_conn);

	auto handle_loop = [](net::conn& c) {
		for (;;) {
			c.handle();
		}
	};

	std::thread server_thread(handle_loop, std::ref(server_conn));
	std::thread proxy_thread(handle_loop, std::ref(proxy_conn));

	while (1) {
	}

	return 0;
}