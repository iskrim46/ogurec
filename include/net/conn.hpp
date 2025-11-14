#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <vector>

#include <netinet/in.h>

#include "net/packet.hpp"
#include "util/io.hpp"

namespace net {
class conn {
	io::serialized_io<io::file_io> rw;

	sockaddr_in conn_addr;
	socklen_t conn_addr_size;

	using handler = std::function<bool(std::span<uint8_t>)>;
	using handler_list = std::vector<handler>;
	std::array<handler_list, 256> handlers;
	std::map<uint16_t, handler_list> netmodule_handlers;

private:
	packet::packet_header read_header()
	{
		uint16_t packet_size;
		uint8_t id;

		rw.read(packet_size);
		rw.read(id);

		return { id, packet_size - 3 };
	}

public:
	conn(int sock_fd, sockaddr_in conn_addr, socklen_t conn_addr_size)
	    : rw(io::file_io(sock_fd))
	    , conn_addr(conn_addr)
	    , conn_addr_size(conn_addr_size)
	{
	}

	template <packet::packet T>
	void expect_packet(T& t)
	{
		auto [id, len] = read_header();
		assert(id == T::packet_id);

		if (len > 0) {
			std::vector<uint8_t> payload(len);
			rw.read(payload);
			decode_packet(payload, t);
		}
	}

	template <class H>
	void reg_handler(int id, H h)
	{
		handlers[id].push_back(h);
	}

	template <packet::packet P, typename H>
	void reg_handler(H h)
	{
		handlers[P::packet_id].push_back([this, h](std::span<uint8_t> payload) {
			P t;
			if (payload.size() > 0) {
				packet::decode_packet(payload, t);
			}
			return h(t);
		});
	}

	template <packet::netmodule P, typename H>
	void reg_handler(H h)
	{
		netmodule_handlers[P::module_id].push_back([this, h](std::span<uint8_t> payload) {
			P t;
			if (payload.size() > 0) {
				packet::decode_packet(payload, t);
			}
			return h(t);
		});
	}

	template <packet::packet T>
	void send_packet(T p)
	{
		send_packet(T::packet_id, encode_packet(p));
	}

	void send_packet(uint8_t id, std::span<uint8_t> payload)
	{
		std::vector<uint8_t> packet_data;
		auto buffer = io::serialized_io(io::buffered_io(packet_data));

		buffer.write(int16_t(payload.size() + 3));
		buffer.write(id);
		if (payload.size() > 0) {
			buffer.write(payload);
		}

		rw.write(packet_data);
	}

	bool handle_netmodule(std::span<uint8_t> payload)
	{
		uint16_t id;
		std::memcpy(&id, payload.data(), sizeof(id));
		id = to_little(id);

		if (netmodule_handlers[id].size() <= 0) {
			// currently i won't bother implementing netmodules
			// FIXME: return false
			return true;
		}

		for (auto& h : handlers[id]) {
			if (h(payload.subspan(2)) == true) {
				return false;
			}
		}
		return true;
	}

	bool handle()
	{
		auto [id, len] = read_header();
		if (id == 0) {
			return false;
		}

		std::vector<uint8_t> payload(len);
		rw.read(payload);

		// got netmodule
		if (id == 82) {
			return handle_netmodule(payload);
		}

		if (handlers[id].size() <= 0) {
			return false;
		}

		for (auto& h : handlers[id]) {
			if (h(payload) == true) {
				break;
			}
		}
		return true;
	}
};
}