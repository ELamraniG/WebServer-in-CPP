#include "../../include/core/Server.hpp"
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <iostream>

void Server::createSocket()
{
	int	opt;

	opt = 1;
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
		throw std::runtime_error("Error: socket failed.");
	if (fcntl(_fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Error: fcntl failed.");
	setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void Server::bindToPort()
{
	struct sockaddr_in server_addr;

	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		throw std::runtime_error("Error: bind failed.");
}

void Server::startListening() const
{
	if (listen(_fd, SOMAXCONN) < 0)
		throw std::runtime_error("Error: listen failed.");
	std::cout << "Server listening on port: " << _port << std::endl;
}

int Server::accept() const
{
	int	clientFd;
	struct sockaddr_in clientAddr;
	socklen_t clientLen;

	clientLen = sizeof(clientLen);
	clientFd = ::accept(_fd, (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd < 0)
	{
		std::cerr << "Error: accept failed." << std::endl;
		return (-1);
	}
	else if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "Error: fcntl failed." << std::endl;
		close(clientFd);
		return (-1);
	}
	return (clientFd);
}

int Server::getFd() const
{
	return (_fd);
}

Server::Server() {}

Server::Server(int port) : _port(port), _fd(-1)
{
	createSocket();
	bindToPort();
	startListening();
}

Server::Server(const Server &obj)
{
	(void)obj;
}

Server& Server::operator=(const Server &obj) 
{
	(void)obj;
	return (*this);
}

Server::~Server()
{
	if (_fd > 0)
		close(_fd);
}

