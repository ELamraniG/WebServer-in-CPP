#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

class Server
{
	private:
		int	_fd;
		int	_port;
		Server();
		Server(const Server &obj);
		Server& operator=(const Server &obj);
		void	createSocket();
		void	bindToPort();
		void	startListening() const;
		
	public:
		~Server();
		Server(int port);
		int	accept() const;
		int	getFd() const;
};
