#include <exception>
#include <vector>

#include "include/config/Server_block.hpp"
#include "include/config/ConfigLoader.hpp"
#include "include/core/Server.hpp"
#include "include/core/ServerFactory.hpp"
#include "include/core/EventLoop.hpp"
#include "include/logger/Logger.hpp"
#include "include/signals/SignalHandler.hpp"

const char*	DEFAULT_CONF = "config/default.conf";

int main(int argc, char **argv)
{
	std::vector<Server*>		serverList;
	std::vector<Server_block>	serverBlocks;

	if (argc > 2)
	{
		Logger::info("Usage: " + std::string(argv[0]) + " [configuration_file]");
		return (1);
	}
	try
	{
		const char*	configFile = (argc == 1 ? DEFAULT_CONF : argv[1]);

		setupSignals();

		serverBlocks = parseConfig(configFile);
		serverList = createServers(serverBlocks);

		EventLoop	eventLoop(serverList, serverBlocks);
		eventLoop.run();

		destroyServers(serverList);
		Logger::info("Shutdown.");
	}
	catch(std::exception& e)
	{
		destroyServers(serverList);
		Logger::fatal(e.what());
		return (1);
	}
	return (0);
}
