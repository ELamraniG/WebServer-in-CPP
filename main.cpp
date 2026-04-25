#include <csignal>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "include/config/Parser.hpp"
#include "include/config/Server_block.hpp"
#include "include/config/Tokenizer.hpp"
#include "include/core/Server.hpp"
#include "include/core/EventLoop.hpp"

bool	g_running = true;

void	signalHandler(int)
{
	g_running = false;
}

void	setupSignals()
{
	struct sigaction	sa;

	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);
}

int main(int argc, char **argv)
{
	std::vector<Server*>		serverList;
	std::vector<Server_block>	serverBlocks;

	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}
	try
	{
		Tokenizer	tokenizer(argv[1]);
		Parser		parser(tokenizer.tokens);

		setupSignals();
		parser.parse();
		serverBlocks = parser.getServers();
		if (serverBlocks.empty())
			throw std::runtime_error("No server blocks found in config");
		for (size_t i = 0; i < serverBlocks.size(); i++)
			serverList.push_back(new Server(serverBlocks[i]));

		EventLoop	eventLoop(serverList, serverBlocks);

		eventLoop.run();
		for (size_t i = 0; i < serverList.size(); i++)
			delete serverList[i];
		std::cout << "[webserv] Shutdown." << std::endl;
	}
	catch(std::exception& e)
	{
		for (size_t i = 0; i < serverList.size(); i++)
			delete serverList[i];
		std::cerr << "[webserv] " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
