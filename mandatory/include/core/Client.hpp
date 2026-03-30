#pragma once

#include <string>
#include <sys/types.h>

class Client
{
	private:
		int _fd;
		std::string	_receiver;
		std::string	_sender;
		time_t		_lastActivity;
		Client();
		Client(const Client &obj);
		Client& operator=(const Client &obj);
		
	public:
		Client(int fd);
		static const int TIMEOUT;
		static const int BUFFER_SIZE;
		int		getFd() const;
		ssize_t	receive();
		ssize_t	send();
		bool	isEmpty() const;
		bool	isTimedOut() const;
		bool	isReqCompleted() const;
		void	eraseConsumedData(int bytes);
		void	updateLastActivity();
		void	setResponse(const std::string &response);
		~Client();
};
