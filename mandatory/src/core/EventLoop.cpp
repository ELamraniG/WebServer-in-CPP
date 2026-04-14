#include "../../include/core/EventLoop.hpp"

#include <cstddef>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <sys/poll.h>

const int EventLoop::POLL_TIMEOUT = 5000;
const int EventLoop::CGI_TIMEOUT = 5;
extern bool g_running;

// TODO: remove this one after CHECK:
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

std::string build500Response()
{
    std::string	body;
	std::string	response;

	body = readFile("mandatory/www/errors/500.html");
    if (body.empty())
    {
		body = readFile("mandatory/www/errors/404.html");
        return "HTTP/1.0 404 Not Found\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: " + toString(body.size()) + "\r\n"
				"\r\n" +
				body;
    }
    response =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + toString(body.size()) + "\r\n"
        "\r\n" +
        body;
    return (response);
}

std::string buildResponse()
{
    std::string	body;
	std::string	response;

	body = readFile("mandatory/www/pages/index.html");
	// body = readFile("mandatory/www/pages/form.html");
    if (body.empty())
    {
		body = readFile("mandatory/www/errors/404.html");
        return "HTTP/1.0 404 Not Found\r\n"
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
// CHECK: ending

bool	EventLoop::isServer(int fd) const
{
	return (_listeningFds.count(fd));
}

void	EventLoop::logEvent(const std::string msg) const
{
	if (VERBOSE && !msg.empty())
		std::cerr << "[webserv] " << msg << "." << std::endl;
}

bool	EventLoop::isReadable(const short revents) const
{
	return (revents & (POLLIN | POLLHUP));
}

bool	EventLoop::isWritable(const short revents) const
{
	return (revents & POLLOUT);
}

bool	EventLoop::isTimeout(int fd)
{
	return (!isServer(fd) && _clientMap.count(fd) && _clientMap[fd]->isTimedOut());
}

bool	EventLoop::isCGITimeout(int fd) const
{
	return (_cgiStartTime.count(fd) && (time(NULL) - _cgiStartTime.at(fd)) > CGI_TIMEOUT);
}

bool	EventLoop::isError(int fd, short revents) const
{
	return (!isServer(fd) && !_cgiFdToHandler.count(fd) && (revents & (POLLERR | POLLHUP)));
}

bool	EventLoop::startCGI(int clientFd)
{
	// all this from moel l'asad
	int									writeFd;
	int									readFd;
    std::string 						path = "./mandatory/www/cgi-bin/test.py"; // correct path
    // std::string 						path = "./mandatory/www/cgi-bin/post_test.py"; // post test script
    // std::string 						path = "./www/cgi-bin/test.py"; // wrong path
    std::string 						method = "GET";
    // std::string 						method = "POST";
    std::string							queryString = "";
    std::string							body = "";
    // std::string						body = "name=REDA";
    std::map<std::string, std::string>	headers;
	CGIHandler 							*cgi;

	// headers["content-type"] = "application/x-www-form-urlencoded";
	// headers["content-length"] = "9";
	cgi = new CGIHandler(path, method, queryString, body, headers);
	if (!cgi->start())
	{
		delete cgi;
		return (false);
	}
	readFd = cgi->getReadFd();
	writeFd = cgi->getWriteFd();
	_cgiStartTime[readFd] = time(NULL);
	_cgiFdToHandler[readFd] = cgi;
	_cgiFdToClient[readFd] = _clientMap[clientFd];
	addToPoll(readFd, POLLIN);
	if (writeFd != -1)
	{
		_cgiFdToHandler[writeFd] = cgi;
		_cgiFdToClient[writeFd] = _clientMap[clientFd];
		addToPoll(writeFd, POLLOUT);
	}
	return (true);
}

void	EventLoop::addToPoll(int fd, short event)
{
	pollfd	pollFd;

	std::memset(&pollFd, 0, sizeof(pollFd));
	pollFd.fd = fd;
	pollFd.events = event;
	_pollFds.push_back(pollFd);
}

void	EventLoop::removeFromPoll(size_t &i)
{
	_pollFds.erase(_pollFds.begin() + i);
	if (i > 0)
		i--;
}

void	EventLoop::handleNewClient(int serverFd)
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
		addToPoll(clientFd, POLLIN);
		_clientMap[clientFd] = new Client(clientFd);
		std::cout << "Client connected" << std::endl;
	}
}

void	EventLoop::handleClientDisconnected(int fd, size_t &i, const std::string &msg)
{
	logEvent(msg);
	delete _clientMap[fd];
	_clientMap.erase(fd);
	removeFromPoll(i);
}

void	EventLoop::handleCGITimeout(int fd, size_t &i)
{
	_cgiFdToHandler[fd]->cleanup();
	_cgiFdToClient[fd]->setResponse(build500Response());
	delete _cgiFdToHandler[fd];
	_cgiFdToHandler.erase(fd);
	_cgiStartTime.erase(fd);
	removeFromPoll(i);
	for (size_t j = 0; j < _pollFds.size(); j++)
	{
		if (_cgiFdToClient[fd]->getFd() == _pollFds[i].fd)
		{
			_pollFds[i].events = POLLOUT;
			break ;
		}
	}
	_cgiFdToClient.erase(fd);
}

void	EventLoop::handleCGIRead(int readFd, size_t &i)
{
	_cgiFdToHandler[readFd]->readOutput();
	if (_cgiFdToHandler[readFd]->isDone() || _cgiFdToHandler[readFd]->isError())
	{
		if (_cgiFdToHandler[readFd]->isError())
			_cgiFdToClient[readFd]->setResponse(build500Response());
		else
			_cgiFdToClient[readFd]->setResponse("HTTP/1.0 200 OK\r\n" + _cgiFdToHandler[readFd]->getOutput());
		for (size_t j = 0; j < _pollFds.size(); j++)
		{
			if (_pollFds[j].fd == _cgiFdToClient[readFd]->getFd())
			{
				_pollFds[j].events = POLLOUT;
				break ;
			}
		}
		removeFromPoll(i);
		delete _cgiFdToHandler[readFd];
		_cgiFdToHandler.erase(readFd);
		_cgiFdToClient.erase(readFd);
		_cgiStartTime.erase(readFd);
	}
}

void	EventLoop::handleRequestComplete(int fd, size_t &i)
{
	bool	isCGI;

    std::string &buffer = _clientMap[fd]->getRequestBuffer();
    isCGI = ((buffer.find(".py") != std::string::npos) || (buffer.find(".php") != std::string::npos)); // TODO: get it from parser of moel l'asad
	if (isCGI)
	{
		if (startCGI(fd))
			_pollFds[i].events = PAUSE;
		else
		{
			_pollFds[i].events = POLLOUT;
			_clientMap[fd]->setResponse(build500Response());
		}
	}
	else
	{
		_pollFds[i].events = POLLOUT;
		_clientMap[fd]->setResponse(buildResponse()); // TODO: also i need it from moel sba3
	}
}

void	EventLoop::handleReadEvent(int fd, size_t &i)
{
	ssize_t	bytes;

	if (isServer(fd))
		handleNewClient(fd);
	else if (_cgiFdToHandler.count(fd))
		handleCGIRead(fd, i);
	else
	{
		bytes = _clientMap[fd]->readFromSocket();
		if (bytes < 0)
			handleClientDisconnected(fd, i, "Error: read");
		else if (bytes == 0)
			handleClientDisconnected(fd, i, "Client disconnected");
		else
		{
			if (_clientMap[fd]->isRequestCompleted())
				handleRequestComplete(fd, i);
			_clientMap[fd]->updateLastActivity();
		}
	}
}

void	EventLoop::handleCGIWrite(int writeFd, size_t &i)
{
	if (_cgiFdToHandler[writeFd]->isError() || _cgiFdToHandler[writeFd]->isWriteBodyDone())
	{
		_cgiFdToHandler.erase(writeFd);
		_cgiFdToClient.erase(writeFd);
		removeFromPoll(i);
	}
	else
		_cgiFdToHandler[writeFd]->writeBody();
}

void	EventLoop::handleWriteEvent(int fd, size_t &i)
{
	ssize_t	bytes;

	if (_cgiFdToHandler.count(fd))
		handleCGIWrite(fd, i);
	else
	{
		bytes = _clientMap[fd]->writeToSocket();
		if (bytes < 0)
			handleClientDisconnected(fd, i, "Error: write");
		else if (_clientMap[fd]->hasNoPendingWrite())
			handleClientDisconnected(fd, i, "");
	}
}

void	EventLoop::run()
{
	int		fd;
	short	revents;

	while (g_running)
	{
		if (poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT) < 0)
		{
			if (!g_running)
				break ;
			throw std::runtime_error("Error: poll.");
		}
		for (size_t i=0; i<_pollFds.size(); i++)
		{
			fd = _pollFds[i].fd;
			revents = _pollFds[i].revents;
			if (isCGITimeout(fd))
				handleCGITimeout(fd, i);
			else if (isTimeout(fd))
				handleClientDisconnected(fd, i, "Client timed out");
			else if (isError(fd, revents))
				handleClientDisconnected(fd, i, "Client disconnected");
			else if (isReadable(revents))
				handleReadEvent(fd, i);
			else if (isWritable(revents))
				handleWriteEvent(fd, i);
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
		addToPoll(fd, POLLIN);
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
	std::map<int, Client*>::iterator		it;
	std::map<int, CGIHandler*>::iterator	cit;
	std::set<CGIHandler*>					deleted;

	for (it = _clientMap.begin(); it != _clientMap.end(); it++)
		delete it->second;
	for (cit = _cgiFdToHandler.begin(); cit != _cgiFdToHandler.end(); cit++)
	{
		if (!deleted.count(cit->second))
		{
			deleted.insert(cit->second);
			delete cit->second;
		}
	}
	// FIXME: i may thinking about create writeFds set to track them instead of store cgi for write and read both
}

/*
	F_OK fails → 404 file not found
	X_OK fails → 403 forbidden, file exists but not executable
*/
