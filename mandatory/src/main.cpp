#include "../include/core/Server.hpp"
#include "../include/core/EventLoop.hpp"
#include <exception>
#include <iostream>
#include <signal.h>
#include <cstdlib>

bool g_running = true;

void handleSigint(int)
{
	g_running = false;
}

int main()
{
	std::vector<Server*>	serverList;

	signal(SIGINT, handleSigint);
	try
	{
		serverList.push_back(new Server(8080));
		serverList.push_back(new Server(8000));
		EventLoop eventloop(serverList);
		eventloop.run();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
	for (size_t i=0; i<serverList.size(); i++)
		delete serverList[i];
	return (0);
}
