#pragma once

#include "../http/HTTPRequest.hpp"

#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

class Client
{
	private:
		Client();
		Client(const Client& obj);
		Client& operator=(const Client& obj);

		int 		_fd;
		std::string	_requestBuffer;
		std::string	_responseBuffer;
		HTTPRequest	_httpRequest;
		time_t		_lastActivity;

	public:
		Client(int fd);
		~Client();

		HTTPRequest			httpReq;
		static const int	TIMEOUT;
		static const int	BUFFER_SIZE;

		int				getFd() const;
		ssize_t			readFromSocket();
		ssize_t			writeToSocket();
		bool			hasNoPendingWrite() const;
		bool			isTimedOut() const;
		bool			isRequestCompleted() const;
		void			eraseConsumedData(int bytes);
		void			updateLastActivity();
		void			setResponse(const std::string& response);
		std::string&	getRequestBuffer();
		HTTPRequest&	getHttpRequest();
};
