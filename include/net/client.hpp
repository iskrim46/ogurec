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

class client {
	struct sockaddr_in server_address {};

public:
	client(std::string ip, int port)
	{
		server_address.sin_family = AF_INET;
		inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr);
		server_address.sin_port = htons(port);
	}

	conn connect()
	{
		int sock_fd;

		sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0)
			throw std::runtime_error(strerror(errno));

		auto err = ::connect(sock_fd, (struct sockaddr*)&server_address,
		    sizeof(server_address));
		if (err < 0)
			throw std::runtime_error(strerror(errno));

		return conn { sock_fd, server_address, sizeof(server_address) };
	}
};
};
