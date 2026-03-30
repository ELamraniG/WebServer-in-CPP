#include "../../include/core/EventLoop.hpp"
#include <cstddef>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

const int EventLoop::POLL_TIMEOUT = 5000;

void EventLoop::addToPoll(int fd)
{
	pollfd	pollFd;

	pollFd.fd = fd;
	pollFd.events = POLLIN;
	_fds.push_back(pollFd);
}

void EventLoop::handleNewConnection(int serverFd)
{
	int	clientFd = -1;

	for (size_t i=0; i<_servers.size(); i++)
	{
		if (_servers[i]->getFd() == serverFd)
		{
			clientFd = _servers[i]->accept();
			break ;
		}
	}
	if (clientFd != -1)
	{
		addToPoll(clientFd);
		_clients[clientFd] = new Client(clientFd);
		std::cout << "Client connected" << std::endl;
	}
}

void EventLoop::handleClientDisconnected(int fd, size_t &i)
{
	delete _clients[fd];
	_clients.erase(fd);
	_fds.erase(_fds.begin() + i);
	i--;
}

void EventLoop::handleRead(int fd, size_t &i)
{
	ssize_t	bytes;

	if (isServer(fd))
		handleNewConnection(fd);
	else
	{
		bytes = _clients[fd]->receive();
		if (bytes < 0)
		{
			log("Error: read");
			handleClientDisconnected(fd, i);
		}
		else if (bytes == 0)
		{
			log("Client disconnected");
			handleClientDisconnected(fd, i);
		}
		else
		{
			if (_clients[fd]->isReqCompleted())
			{
				_fds[i].events = POLLOUT;
				_clients[fd]->setResponse(RESPONSE);
			}
			_clients[fd]->updateLastActivity();
		}
	}
}

void EventLoop::handleWrite(int fd, size_t &i)
{
	ssize_t	bytes;

	bytes = _clients[fd]->send();
	if (bytes < 0)
	{
		log("Error: read");
		handleClientDisconnected(fd, i);
	}
	else if (_clients[fd]->isEmpty())
		handleClientDisconnected(fd, i);
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
		for (size_t i=0; i<_fds.size(); i++)
		{
			if (!isServer(_fds[i].fd) && _clients[_fds[i].fd]->isTimedOut())
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
				handleRead(_fds[i].fd, i);
			else if (isWritable(_fds[i].revents))
				handleWrite(_fds[i].fd, i);
		}
	}
}

EventLoop::EventLoop() {}

EventLoop::EventLoop(const std::vector<Server*> &servers) :
	_servers(servers)
{
	int	fd;

	for (size_t i=0; i<servers.size(); i++)
	{
		fd = servers[i]->getFd();
		_serverFds.insert(fd);
		addToPoll(fd);
	}
}

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
	std::map<int, Client*>::iterator	it;

	for (it = _clients.begin(); it != _clients.end(); it++)
		delete it->second;
	for (size_t i=0; i<_servers.size(); i++)
		delete _servers[i];
}
