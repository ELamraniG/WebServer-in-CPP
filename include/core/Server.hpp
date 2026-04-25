#pragma once

#include "../config/Server_block.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

class Server
{
	private:
		Server();
		Server(const Server& obj);
		Server& operator=(const Server& obj);

		int				_fd;
		Server_block	_serverBlock;

		void	createSocket();
		void	bindSocket();
		void	startListening() const;

	public:
		~Server();
		Server(const Server_block& serverBlock);

		int	accept() const;
		int	getFd() const;
};
