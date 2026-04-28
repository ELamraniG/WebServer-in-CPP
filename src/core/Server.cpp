#include "../../include/core/Server.hpp"
#include "../../include/logger/Logger.hpp"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <netdb.h>

extern Logger	logger;

int	Server::getFd() const
{
	return (_fd);
}

int	Server::accept() const
{
	int					clientFd;
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;

	clientLen = sizeof(clientAddr);
	clientFd = ::accept(_fd, (struct sockaddr*)&clientAddr, &clientLen);
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

void	Server::startListening() const
{
	if (listen(_fd, SOMAXCONN) < 0)
		throw std::runtime_error("Error: listen failed.");
	logger.serverStart(_serverBlock.host, _serverBlock.port, _fd);
}

void	Server::bindSocket()
{
	struct sockaddr_in	serverAddr;
	struct addrinfo		hints;
	struct addrinfo*	res;
	int					portNum;
	std::string			host;

	std::memset(&serverAddr, 0, sizeof(serverAddr));
	portNum = std::atoi(_serverBlock.port.c_str());
	host = _serverBlock.host;
	if (host == "0.0.0.0")
	{
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(portNum);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(host.c_str(), _serverBlock.port.c_str(), &hints, &res) != 0)
			throw std::runtime_error("Error: getaddinfo failed.");
		serverAddr = *(struct sockaddr_in *)res->ai_addr;
		freeaddrinfo(res);
	}
	if (bind(_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("Error: bind failed.");
}

void	Server::createSocket()
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

Server::Server() {}

Server::Server(const Server_block& serverBlock) :
	_fd(-1),
	_serverBlock(serverBlock)
{
	createSocket();
	bindSocket();
	startListening();
}

Server::Server(const Server& obj)
{
	(void)obj;
}

Server& Server::operator=(const Server& obj) 
{
	(void)obj;
	return (*this);
}

Server::~Server()
{
	if (_fd >= 0)
		close(_fd);
}
