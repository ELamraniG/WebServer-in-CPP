#pragma once

#include "../config/Server_block.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
// #include <string>

class Server
{
	private:
		Server();
		Server(const Server& obj);
		Server& operator=(const Server& obj);

		int				_fd;
		Server_block	_serverBlock;
		// int			_port;
		// std::string	_ipBind;

		void	createSocket();
		void	bindSocket();
		void	startListening() const;

	public:
		~Server();
		Server(const Server_block& serverBlock);
		// Server(int port, std::string ipBind);

		int	accept() const;
		int	getFd() const;
};
