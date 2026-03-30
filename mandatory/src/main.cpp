#include "../include/core/Server.hpp"
#include "../include/core/EventLoop.hpp"
#include <exception>
#include <iostream>
#include <vector>

int main()
{
	try
	{
		std::vector<Server*>	servers;

		servers.push_back(new Server(8080));
		EventLoop eventloop(servers);
		eventloop.run();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return (0);
}
