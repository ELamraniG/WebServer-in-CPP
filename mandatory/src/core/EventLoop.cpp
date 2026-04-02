#include "../../include/core/EventLoop.hpp"
#include <iostream>
#include <stdexcept>

const int EventLoop::POLL_TIMEOUT = 5000;
extern bool g_running;

// TODO: remove this one after
#include <fstream>
#include <sstream>
#include <string>

std::string toString(size_t n)
{
    std::stringstream	ss;

    ss << n;
    return (ss.str());
}

std::string readFile(const std::string& path)
{
	std::ifstream		file(path.c_str(), std::ios::in | std::ios::binary);
    std::ostringstream	ss;

    if (!file)
        return ("");
    ss << file.rdbuf();
    return (ss.str());
}
// TODO: ending

std::string buildResponse()
{
    std::string	body;
	std::string	response;

	body = readFile("mandatory/www/pages/index.html");
    if (body.empty())
    {
        body = "<html><body><h1>404 Not Found</h1></body></html>";
        return "HTTP/1.1 404 Not Found\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: " + toString(body.size()) + "\r\n"
				"\r\n" +
				body;
    }
    response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + toString(body.size()) + "\r\n"
        "\r\n" +
        body;
    return (response);
}

bool EventLoop::isServer(int fd) const
{
	return (_listeningFds.count(fd));
}

void EventLoop::logEvent(const std::string msg) const
{
	if (VERBOSE && !msg.empty())
		std::cerr << "[webserv] " << msg << "." << std::endl;
}

bool EventLoop::isReadable(const short revents) const
{
	return (revents & POLLIN);
}

bool EventLoop::isWritable(const short revents) const
{
	return (revents & POLLOUT);
}

bool EventLoop::isTimeout(int i)
{
	return (!isServer(_pollFds[i].fd) && _clientMap.count(_pollFds[i].fd) && _clientMap[_pollFds[i].fd]->isTimedOut());
}

bool EventLoop::isError(int i) const
{
	return (!isServer(_pollFds[i].fd) && (_pollFds[i].revents & (POLLERR | POLLHUP)));
}

void EventLoop::addToPoll(int fd)
{
	pollfd	pollFd;

	std::memset(&pollFd, 0, sizeof(pollFd));
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

void EventLoop::handleClientDisconnected(int fd, size_t &i, const std::string &msg)
{
	logEvent(msg);
	delete _clientMap[fd];
	_clientMap.erase(fd);
	_pollFds.erase(_pollFds.begin() + i);
	if (i > 0)
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
			handleClientDisconnected(fd, i, "Error: read");
		else if (bytes == 0)
			handleClientDisconnected(fd, i, "Client disconnected");
		else
		{
			if (_clientMap[fd]->isReqCompleted())
			{
				_pollFds[i].events = POLLOUT;
				_clientMap[fd]->setResponse(buildResponse());
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
		handleClientDisconnected(fd, i, "Error: write");
	else if (_clientMap[fd]->hasNoPendingWrite())
		handleClientDisconnected(fd, i, "");
}

void EventLoop::run()
{
	int	ret;

	while (g_running)
	{
		ret = poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT);
		if (ret < 0)
		{
			if (!g_running)
				break ;
			throw std::runtime_error("Error: poll.");
		}
		for (size_t i=0; i<_pollFds.size(); i++)
		{
			if (isTimeout(i))
				handleClientDisconnected(_pollFds[i].fd, i, "Client timed out");
			else if (isError(i))
				handleClientDisconnected(_pollFds[i].fd, i, "Client disconnected");
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
