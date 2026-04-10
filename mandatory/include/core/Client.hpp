#pragma once

#include <string>
#include <sys/types.h>

class Client
{
	private:
		Client();
		Client(const Client &obj);
		Client& operator=(const Client &obj);

		int 		_fd;
		std::string	_requestBuffer;
		std::string	_responseBuffer;
		time_t		_lastActivity;

	public:
		Client(int fd);
		~Client();

		static const int TIMEOUT;
		static const int BUFFER_SIZE;

		int		getFd() const;
		ssize_t	readFromSocket();
		ssize_t	writeToSocket();
		bool	hasNoPendingWrite() const;
		bool	isTimedOut() const;
		bool	isRequestCompleted() const;
		void	eraseConsumedData(int bytes);
		void	updateLastActivity();
		void	setResponse(const std::string &response);
};
