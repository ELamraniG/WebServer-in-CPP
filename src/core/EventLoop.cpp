#include "../../include/core/EventLoop.hpp"
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>

const int EventLoop::POLL_TIMEOUT = 5000;

void EventLoop::handleNewConnection(int fd)
{
	int					clientFd;
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;
	pollfd				clientPoll;

	clientFd = accept(fd,  (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd < 0)
	{
		log("Error: accept");
		return ;
	}
	else if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		log("Error: fcntl(F_SETFL)");
		close(clientFd);
		return ;
	}
	clientPoll.fd = clientFd;
	clientPoll.events = POLLIN;
	_fds.push_back(clientPoll);
	_clients[clientFd] = new Client(fd);
	std::cout << "Client connected" << std::endl;
}

void EventLoop::handleClientDisconnected(int fd, int &i)
{
	close(fd);
	delete _clients[fd];
	_clients.erase(fd);
	_servers.erase(_servers.begin() + i);
	_serverFds.erase(fd);
	_fds.erase(_fds.begin() + i);
	i--;
}

void EventLoop::handleRead(int fd)
{
	int					clientFd;
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;
	pollfd				clientPoll;

	if (isServer(fd))
		handleNewConnection(fd);
	else
		
}

void EventLoop::handleWrite()
{
	
}

bool EventLoop::isServer(int fd) const
{
	return (_serverFds.count(fd));
}

void EventLoop::log(const std::string msg) const
{
	if (VERBOSE)
		std::cerr << "[webserv] " << msg << "." << std::endl;
}

bool EventLoop::isError(const short revents) const
{
	return (revents & (POLLERR | POLLHUP));
}

bool EventLoop::isReadable(const short revents) const
{
	return (revents & POLLIN);
}

bool EventLoop::isWritable(const short revents) const
{
	return (revents & POLLOUT);
}

void EventLoop::run()
{
	int	ret;

	while (true)
	{
		ret = poll(_fds.data(), _fds.size(), POLL_TIMEOUT);
		if (ret < 0)
			throw std::runtime_error("Error: poll.");
		for (int i=0; i<_fds.size(); i++)
		{
			if (isServer(_fds[i].fd) && _clients[i]->isTimedOut())
			{
				log("Client timed out");
				handleClientDisconnected(_fds[i].fd, i);
			}
			else if (!isServer(_fds[i].fd) && isError(_fds[i].revents))
			{
				log("Client disconnected");
				handleClientDisconnected(_fds[i].fd, i);
			}
			else if (isReadable(_fds[i].revents))
			{
				handleRead(_fds[i].fd);
			}
		}
	}
}

EventLoop::EventLoop() {}

EventLoop::EventLoop(const EventLoop &obj)
{
	(void)obj;
}

EventLoop& EventLoop::operator=(const EventLoop &obj) 
{
	(void)obj;
	return (*this);
}

EventLoop::~EventLoop()
{
	for (int i=0; i<_fds.size(); i++)
		close(_fds[i].fd);
	// need to close all remaining clients in _client map with delete.
}
