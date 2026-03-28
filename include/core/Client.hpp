#pragma once


#include <string>
#include <sys/types.h>

class Client
{
	private:
		int _fd;
		std::string	_receiver;
		std::string	_sender;
		ssize_t		_bytesSent;
		time_t		_lastActivity;
		Client();
		Client(int fd);
		Client(const Client &obj);
		Client& operator=(const Client &obj);
		
	public:
		static const int TIMEOUT;
		static const int BUFFER_SIZE;
		static const int MAX_HEADER_SIZE;
		int		getFd() const;
		ssize_t	receive();
		ssize_t	send();
		bool	isTimedOut();
		bool	isReqCompleted();
		void	eraseConsumedData();
		void	updateLastActivity();
		~Client();
};
