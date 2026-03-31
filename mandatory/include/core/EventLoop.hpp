#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include <sys/poll.h>
#include <map>
#include <set>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

const bool VERBOSE = true;
const int BUFFER_SIZE = 4096;
// const std::string RESPONSE("HTTP/1.0 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");

class EventLoop
{
	private:
		std::vector<pollfd>		_pollFds;
		std::vector<Server*>	_serverList;
		std::set<int>			_listeningFds;
		std::map<int, Client*>	_clientMap;
		EventLoop();
		EventLoop(const EventLoop &obj);
		EventLoop& operator=(const EventLoop &obj);
		void addToPoll(int fd);
		void handleNewClient(int fd);
		void handleClientDisconnected(int fd, size_t &i, const std::string &msg);
		void handleReadEvent(int fd, size_t &i);
		void handleWriteEvent(int fd, size_t &i);
		bool isTimeout(int i);
		bool isError(int i) const;
		bool isServer(int fd) const;
		void handleError();
		void logEvent(const std::string msg) const;
		bool isReadable(const short revents) const;
		bool isWritable(const short revents) const;

	public:
		static const int POLL_TIMEOUT;
		EventLoop(const std::vector<Server*> &servers);
		~EventLoop();
		void run();
};
