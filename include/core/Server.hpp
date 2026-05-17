#pragma once

#include "../config/Server_block.hpp"
#include "../core/SocketGuard.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

class Server
{
	private:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);

		SocketGuard		_socket;
		Server_block	_config;

		void				createSocket();
		struct sockaddr_in	buildAddress();
		struct sockaddr_in	buildAnyAddress(int port);
		struct sockaddr_in	resolveAddress(const char* host, const char* port);
		void				bindSocket();
		void				startListening() const;

	public:
		~Server();
		Server(const Server_block& serverBlock);

		int					accept() const;
		int					getFd() const;
		const Server_block&	getConfig() const;
};
