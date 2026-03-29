#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include <map>
#include <set>
#include <vector>

const bool VERBOSE = true;

class EventLoop
{
	private:
		std::vector<pollfd>		_fds;
		std::vector<Server>		_servers;
		std::set<int>			_serverFds;
		std::map<int, Client*>	_clients;
		EventLoop();
		EventLoop(const EventLoop &obj);
		EventLoop& operator=(const EventLoop &obj);
		void handleNewConnection(int fd);
		void handleClientDisconnected(int fd, int &i);
		void handleRead(int fd);
		void handleWrite();
		bool isServer(int fd) const;
		void handleError();
		void log(const std::string msg) const;
		bool isError(const short revents) const;
		bool isReadable(const short revents) const;
		bool isWritable(const short revents) const;

	public:
		static const int POLL_TIMEOUT;
		~EventLoop();
		void run();
};

// handle new connection
// check timeout
// handle read
// handle client disconnected
// handle error
// handle write