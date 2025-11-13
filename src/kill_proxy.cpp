#include <arpa/inet.h>
#include <bitset>
#include <concepts>
#include <cstdint>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "net/client.hpp"
#include "net/conn.hpp"
#include "net/server.hpp"
#include "net/types.hpp"

using net::packet::operator""_ns;

template <typename T, T... ints>
void forward_packets(net::conn& dst, net::conn& src, std::integer_sequence<T, ints...> int_seq)
{
	(src.reg_handler<net::packet::raw<ints+1>>([&dst](auto& a) {
		dst.send_packet(a);
		return true;
	}),
	    ...);
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
	server_conn.reg_handler<net::packet::accept>([=, &slot](auto& a) {
		slot = a.client_id;
		return false;
	});

	proxy_conn.reg_handler<net::packet::damage_player>([&slot, &server_conn](auto& dmg) {
		if (dmg.client_id != slot) {
			dmg.damage *= 100;
			dmg.reason.reasons.at(0) = 0;

			dmg.reason.reasons[net::packet::death_reason::reason::Custom] = true;
			dmg.reason.custom = "killed by server insecurity";

			server_conn.send_packet(dmg);
			return true;
		}
		return false;
	});

	forward_packets(server_conn, proxy_conn, std::make_integer_sequence<uint8_t, 255> {});
	forward_packets(proxy_conn, server_conn, std::make_integer_sequence<uint8_t, 255> {});

	auto handle_loop = [](net::conn& c) {
		for (;;) {
			c.handle();
		}
	};

	std::thread server_thread(handle_loop, std::ref(server_conn));
	std::thread proxy_thread(handle_loop, std::ref(proxy_conn));

	return 0;
}