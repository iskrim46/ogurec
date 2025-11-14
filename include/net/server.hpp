#pragma once

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "net/conn.hpp"

namespace net {

class server {
	struct sockaddr_in server_address {};
	int sock_fd;

public:
	server(std::string hostname, int port)
	{
		server_address.sin_family = AF_INET;
		inet_pton(AF_INET, hostname.c_str(), &server_address.sin_addr);
		server_address.sin_port = htons(port);
	}

	~server()
	{
		close(sock_fd);
	}

	void bind()
	{
		sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0)
			throw std::runtime_error(strerror(errno));

		int bindStatus = ::bind(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
		if (bindStatus < 0)
			throw std::runtime_error(strerror(errno));
	}

	void listen(int max_connections)
	{
		::listen(sock_fd, max_connections);
	}

	conn accept()
	{
		sockaddr_in conn_addr {};
		socklen_t conn_addr_size = sizeof(conn_addr);

		int conn_fd = ::accept(sock_fd, (struct sockaddr*)&conn_addr, &conn_addr_size);
		if (conn_fd < 0)
			throw std::runtime_error(strerror(errno));
		return conn { conn_fd, conn_addr, conn_addr_size };
	}
};
}