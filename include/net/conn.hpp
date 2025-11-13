#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <span>
#include <vector>

#include "net/packet.hpp"

namespace net {
class conn {
	int sock_fd;

	sockaddr_in conn_addr;
	socklen_t conn_addr_size;

    using handler_list = std::vector<std::function<bool(std::span<uint8_t>)>>;
    std::array<handler_list, 256> handlers;
    std::map<uint16_t, handler_list> netmodule_handlers;

private:
	packet::packet_header read_header()
	{
		uint16_t packet_size;
		uint8_t id;

		recv(packet_size);
		recv(id);

		return { id, packet_size - 3};
	}

public:
	conn(int sock_fd, sockaddr_in conn_addr, socklen_t conn_addr_size)
	: sock_fd(sock_fd)
    , conn_addr(conn_addr)
    , conn_addr_size(conn_addr_size)
	{
	}

	~conn()
	{
		close(sock_fd);
	}

	ssize_t send(std::span<uint8_t> data)
	{
		auto bytes = ::send(sock_fd, data.data(), data.size_bytes(), 0);
		if (bytes < 0) {
			throw std::runtime_error(strerror(errno));
		}
		return bytes;
	}

	template <std::integral T>
	ssize_t send(T t)
	{
		t = to_little(t);
		auto bytes = ::send(sock_fd, &t, sizeof(T), 0);
		if (bytes < 0) {
			throw std::runtime_error(strerror(errno));
		}
		return bytes;
	}

	ssize_t recv(std::span<uint8_t> data)
	{
        if (data.size_bytes() == 0) {
            return  0;
        }

		auto bytes = ::recv(sock_fd, data.data(), data.size_bytes(), 0);
		if (bytes <= 0) {
			throw std::runtime_error(strerror(errno));
		}
		return bytes;
	}

	template <std::integral T>
	ssize_t recv(T& t)
	{
		auto bytes = ::recv(sock_fd, &t, sizeof(T), 0);
		if (bytes <= 0) {
			throw std::runtime_error(strerror(errno));
		}
		t = to_little(t);
		if (bytes != sizeof(T)) {
			throw std::runtime_error(strerror(errno));
		}
		return bytes;
	}

	template <packet::packet T>
	void expect_packet(T& t)
	{
		auto [id, len] = read_header();
		assert(id == T::packet_id);

		if(len > 0) {
			std::vector<uint8_t> payload(len);
			recv(payload);
			decode_packet(payload, t);
		}
	}

    template <packet::packet P, typename H>
    void reg_handler(H h) {
        handlers[P::packet_id].push_back([this, h](std::span<uint8_t> payload) {
            P t;
            if (payload.size() > 0) {
				packet::decode_packet(payload,  t);
			}
            return h(t);
        });
    }

    template <packet::netmodule P, typename H>
    void reg_handler(H h) {
        netmodule_handlers[P::module_id].push_back([this, h](std::span<uint8_t> payload) {
            P t;
            if (payload.size() > 0) {
				packet::decode_packet(payload,  t);
			}
            return h(t);
        });
    }

	template <packet::packet T>
	void send_packet(T p)
	{
		auto payload = encode_packet(p);

		send(int16_t(payload.size() + 3));
		send(T::packet_id);
		if (payload.size() > 0) {
			send(payload);
		}
    }
    
	bool handle_netmodule(std::span<uint8_t> payload) {
		uint16_t id;
		std::memcpy(&id, payload.data(), sizeof(id));
		id = to_little(id);

        if (netmodule_handlers[id].size() <= 0) {
			// currently i won't bother implementing netmodules
			// FIXME: return false
        	return true;
        }

        for (auto &h : handlers[id]) {
            if (h(payload.subspan(2)) == true) {
                return false;
            }
        }
        return true;
	}

    bool handle() {
        auto [id, len] = read_header();
        if (id == 0) {
            return false;
        }

        std::vector<uint8_t> payload(len);
        recv(payload);

		//got netmodule
        if (id == 82) {
			return handle_netmodule(payload);
		}

        if (handlers[id].size() <= 0) {
          return false;
        }

        for (auto &h : handlers[id]) {
            if (h(payload) == true) {
                break;
            }
        }
        return true;
    }

};
}