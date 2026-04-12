#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include "../cgi/CGIHandler.hpp"
#include <map>
#include <set>
#include <vector>
#include <cstddef>
#include <cstring>

const bool	VERBOSE = true;
const int	BUFFER_SIZE = 4096;
const int	PAUSE = 0;

class EventLoop
{
	private:
		std::vector<pollfd>			_pollFds;
		std::vector<Server*>		_serverList;
		std::set<int>				_listeningFds;
		std::map<int, Client*>		_clientMap;
		std::map<int, CGIHandler*>	_cgiFdToHandler;
		std::map<int, Client*>		_cgiFdToClient;

		EventLoop();
		EventLoop(const EventLoop &obj);
		EventLoop& operator=(const EventLoop &obj);

		void	addToPoll(int fd, short event);
		void	removeFromPoll(size_t &i);
		void	handleNewClient(int fd);
		void	handleClientDisconnected(int fd, size_t &i, const std::string &msg);
		void	handleReadEvent(int fd, size_t &i);
		void	handleWriteEvent(int fd, size_t &i);
		bool	isTimeout(int i);
		bool	isError(int i) const;
		bool	isServer(int fd) const;
		void	handleError();
		void	logEvent(const std::string msg) const;
		bool	isReadable(const short revents) const;
		bool	isWritable(const short revents) const;
		void	startCGI(int clientFd);
		void	handleCGIRead(int readFd, size_t &i);
		void	handleCGIWrite(int writeFd, size_t &i);
		void	handleRequestComplete(int fd, size_t &i);
		void	handleCGITimeout();

	public:
		EventLoop(const std::vector<Server*> &servers);
		~EventLoop();

		static const int	POLL_TIMEOUT;

		void	run();
};
