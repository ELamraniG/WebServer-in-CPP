#pragma once

#include "Client.hpp"
// #include "Server.hpp"
#include <sys/poll.h>
#include <map>
#include <set>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

const bool VERBOSE = true;
const int BUFFER_SIZE = 4096;
const std::string RESPONSE("HTTP/1.0 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");

class EventLoop
{
	private:
		std::vector<pollfd>		_fds;
		// std::vector<Server>		_servers;
		std::set<int>			_serverFds;
		std::map<int, Client*>	_clients;
		EventLoop();
		EventLoop(const EventLoop &obj);
		EventLoop& operator=(const EventLoop &obj);
		void addToPoll(int fd);
		void handleNewConnection(int fd);
		void handleClientDisconnected(int fd, size_t &i);
		void handleRead(int fd, size_t &i);
		void handleWrite(int fd, size_t &i);
		bool isServer(int fd) const;
		void handleError();
		void log(const std::string msg) const;
		bool isError(const short revents) const;
		bool isReadable(const short revents) const;
		bool isWritable(const short revents) const;

	public:
		static const int POLL_TIMEOUT;
		EventLoop(const std::set<int> &severFds);
		~EventLoop();
		void run();
};
