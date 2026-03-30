#include "../include/core/Server.hpp"
#include "../include/core/EventLoop.hpp"
#include <set>

int main()
{
	Server server(8080);
	std::set<int> serverFds;
	serverFds.insert(server.getFd());
	EventLoop eventloop(serverFds);
	return (0);
}
