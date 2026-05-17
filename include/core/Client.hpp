#pragma once

#include "../http/HTTPRequest.hpp"
#include "../core/Server.hpp"

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
		Client(const Client& other);
		Client& operator=(const Client& other);

		int 		_fd;
		Server*		_server;
		std::string	_requestBuffer;
		std::string	_responseBuffer;
		time_t		_lastActivity;

	public:
		Client(int fd, Server* server);
		~Client();

		HTTPRequest	httpReq;

		int				getFd() const;
		const Server*	getServer() const;
		ssize_t			readFromSocket();
		ssize_t			writeToSocket();
		bool			hasNoPendingWrite() const;
		bool			isTimedOut() const;
		bool			isRequestCompleted() const;
		void			eraseConsumedData(int bytes);
		void			updateLastActivity();
		void			setResponse(const std::string& response);
		std::string&	getRequestBuffer();
};
