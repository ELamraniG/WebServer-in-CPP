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
	_pollFds.push_back(pollFd);
}

void EventLoop::handleNewClient(int serverFd)
{
	int	clientFd = -1;

	for (size_t i=0; i<_serverList.size(); i++)
	{
		if (_serverList[i]->getFd() == serverFd)
		{
			clientFd = _serverList[i]->accept();
			break ;
		}
	}
	if (clientFd != -1)
	{
		addToPoll(clientFd);
		_clientMap[clientFd] = new Client(clientFd);
		std::cout << "Client connected" << std::endl;
	}
}

void EventLoop::handleClientDisconnected(int fd, size_t &i)
{
	delete _clientMap[fd];
	_clientMap.erase(fd);
	_pollFds.erase(_pollFds.begin() + i);
	i--;
}

void EventLoop::handleReadEvent(int fd, size_t &i)
{
	ssize_t	bytes;

	if (isServer(fd))
		handleNewClient(fd);
	else
	{
		bytes = _clientMap[fd]->readFromSocket();
		if (bytes < 0)
		{
			logEvent("Error: read");
			handleClientDisconnected(fd, i);
		}
		else if (bytes == 0)
		{
			logEvent("Client disconnected");
			handleClientDisconnected(fd, i);
		}
		else
		{
			if (_clientMap[fd]->isReqCompleted())
			{
				_pollFds[i].events = POLLOUT;
				_clientMap[fd]->setResponse(RESPONSE);
			}
			_clientMap[fd]->updateLastActivity();
		}
	}
}

void EventLoop::handleWriteEvent(int fd, size_t &i)
{
	ssize_t	bytes;

	bytes = _clientMap[fd]->writeToSocket();
	if (bytes < 0)
	{
		logEvent("Error: write");
		handleClientDisconnected(fd, i);
	}
	else if (_clientMap[fd]->hasNoPendingWrite())
		handleClientDisconnected(fd, i);
}

bool EventLoop::isServer(int fd) const
{
	return (_listeningFds.count(fd));
}

void EventLoop::logEvent(const std::string msg) const
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
		ret = poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT);
		if (ret < 0)
			throw std::runtime_error("Error: poll.");
		for (size_t i=0; i<_pollFds.size(); i++)
		{
			if (!isServer(_pollFds[i].fd) && _clientMap.count(_pollFds[i].fd) && _clientMap[_pollFds[i].fd]->isTimedOut())
			{
				logEvent("Client timed out");
				handleClientDisconnected(_pollFds[i].fd, i);
			}
			else if (!isServer(_pollFds[i].fd) && isError(_pollFds[i].revents))
			{
				logEvent("Client disconnected");
				handleClientDisconnected(_pollFds[i].fd, i);
			}
			else if (isReadable(_pollFds[i].revents))
				handleReadEvent(_pollFds[i].fd, i);
			else if (isWritable(_pollFds[i].revents))
				handleWriteEvent(_pollFds[i].fd, i);
		}
	}
}

EventLoop::EventLoop() {}

EventLoop::EventLoop(const std::vector<Server*> &serverList) :
	_serverList(serverList)
{
	int	fd;

	for (size_t i=0; i<serverList.size(); i++)
	{
		fd = serverList[i]->getFd();
		_listeningFds.insert(fd);
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

	for (it = _clientMap.begin(); it != _clientMap.end(); it++)
		delete it->second;
}
