#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include <map>
#include <vector>

class EventLoop
{
	private:
		std::vector<pollfd>		_fds;
		std::vector<Server>		_servers;
		std::map<int, Client>	_clients;
		EventLoop();
		EventLoop(const EventLoop &obj);
		EventLoop& operator=(const EventLoop &obj);

	public:
		~EventLoop();
};

// cleanup fn
