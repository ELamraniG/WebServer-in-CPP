#include "../../include/core/Client.hpp"

#include <cstddef>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

const int Client::TIMEOUT = 5;
const int Client::BUFFER_SIZE = 4096;

ssize_t Client::receive()
{
	char	buffer[BUFFER_SIZE];
	ssize_t	bytes;

	bytes = read(_fd, buffer, BUFFER_SIZE);
	if (bytes > 0)
		_receiver.append(buffer, bytes);
	return (bytes);
}

ssize_t Client::send()
{
	size_t	responseSize;
	ssize_t	bytes;

	responseSize = _sender.size();
	bytes = write(_fd, _sender.c_str(), responseSize);
	if (bytes > 0)
		eraseConsumedData(bytes);
	return (bytes);
}

int Client::getFd() const
{
	return (_fd);
}

bool Client::isEmpty() const
{
	return (_sender.size() == 0);
}

void Client::setResponse(const std::string &response)
{
	_sender = response;
}

bool Client::isTimedOut() const
{
	return ((time(NULL) - _lastActivity) > TIMEOUT);
}

bool Client::isReqCompleted() const
{
	return (_receiver.find("\r\n\r\n") != std::string::npos);
}

void Client::eraseConsumedData(int bytes)
{
	_sender.erase(0, bytes);
}

void Client::updateLastActivity()
{
	_lastActivity = time(NULL);
}

Client::Client() {}

Client::Client(int fd) :
	_fd(fd),
	_receiver(""),
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
