#include "../../include/core/Server.hpp"
#include "../../include/logger/Logger.hpp"

#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <netdb.h>
#include <unistd.h>

extern Logger	logger;

int	Server::getFd() const
{
	return (_socket.fd);
}

const Server_block&	Server::getConfig() const
{
	return (_config);
}

int	Server::accept() const
{
	int					clientFd;
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;

	clientLen = sizeof(clientAddr);
	clientFd = ::accept(_socket.fd, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientFd < 0)
	{
		Logger::error("accept failed.");
		return (-1);
	}
	else if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		Logger::error("fcntl failed.");
		close(clientFd);
		return (-1);
	}
	return (clientFd);
}

void	Server::startListening() const
{
	if (listen(_socket.fd, SOMAXCONN) < 0)
		throw std::runtime_error("Error: listen failed.");
	logger.serverStart(_config.host, _config.port, _socket.fd);
}

struct sockaddr_in	Server::resolveAddress(const char* host, const char* port)
{
	struct sockaddr_in	serverAddr;
	struct addrinfo		hints;
	struct addrinfo*	res;

	std::memset(&serverAddr, 0, sizeof(serverAddr));
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		throw std::runtime_error("Error: getaddinfo failed.");
	serverAddr = *(struct sockaddr_in *)res->ai_addr;
	freeaddrinfo(res);
	return (serverAddr);
}

struct sockaddr_in	Server::buildAnyAddress(int port)
{
	struct sockaddr_in	serverAddr;

	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	return (serverAddr);
}

struct sockaddr_in	Server::buildAddress()
{
	int			port;
	std::string	host;

	port = std::atoi(_config.port.c_str());
	host = _config.host;
	if (host == "0.0.0.0")
		return (buildAnyAddress(port));
	return (resolveAddress(host.c_str(), _config.port.c_str()));
}

void	Server::bindSocket()
{
	struct sockaddr_in	serverAddr;

	serverAddr = buildAddress();
	if (bind(_socket.fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("bind failed on " + _config.host + ":" + _config.port);
}

void	Server::createSocket()
{
	int	opt;

	opt = 1;
	_socket.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket.fd < 0)
		throw std::runtime_error("Error: socket failed.");
	if (fcntl(_socket.fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Error: fcntl failed.");
	setsockopt(_socket.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

Server::Server() {}

Server::Server(const Server_block& serverBlock) :
	_config(serverBlock)
{
	createSocket();
	bindSocket();
	startListening();
}

Server::Server(const Server& other)
{
	(void) other;
}

Server& Server::operator=(const Server& other) 
{
	(void) other;
	return (*this);
}

Server::~Server() {}
