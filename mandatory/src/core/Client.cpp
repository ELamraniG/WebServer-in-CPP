#include "../../include/core/Client.hpp"

#include <cstddef>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

const int Client::TIMEOUT = 55;
const int Client::BUFFER_SIZE = 4096;

ssize_t Client::receive()
{
	std::string	buffer;
	ssize_t		bytes;

	bytes = read(_fd, &buffer[0], BUFFER_SIZE);
	if (bytes > 0)
	{
		bytes += _receiver.size();
		_receiver += buffer;
	}
	return (bytes);
}

ssize_t Client::send()
{
	size_t	response_size;
	ssize_t	bytes;

	response_size = _sender.size();
	bytes = write(_fd, _sender.c_str(), response_size);
	if (bytes > 0)
	{
		_sender.erase(0, bytes);
		_bytesSent += bytes;
	}
	return (_bytesSent);
}

int Client::getFd() const
{
	return (_fd);
}

int Client::getSenderSize() const
{
	return (_sender.size());
}

void Client::setResponse(const std::string &response)
{
	_sender = response;
}

bool Client::isTimedOut()
{
	return ((time(NULL) - _lastActivity) > TIMEOUT);
}

bool Client::isReqCompleted()
{
	return (_receiver.find("\r\n\r\n") != std::string::npos);
}

void Client::eraseConsumedData()
{
	_sender.erase(0, _bytesSent);
}

void Client::updateLastActivity()
{
	_lastActivity = time(NULL);
}

Client::Client() {}

Client::Client(int fd) :
	_fd(fd),
	_receiver(""),
	_bytesSent(0),
	_lastActivity(time(NULL))
{}

Client::Client(const Client &obj)
{
	(void)obj;
}

Client& Client::operator=(const Client &obj) 
{
	(void)obj;
	return (*this);
}

Client::~Client()
{
	if (_fd > 0)
		close(_fd);
}
