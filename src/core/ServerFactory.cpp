#include "../../include/core/ServerFactory.hpp"
#include "../../include/core/Server.hpp"
#include "../../include/logger/Logger.hpp"

#include <stdexcept>
#include <exception>

std::vector<Server*>	createServers(const std::vector<Server_block>& serverBlocks)
{
	std::vector<Server*>	serverList;

	for (size_t i = 0; i < serverBlocks.size(); i++)
	{
		try
		{
			serverList.push_back(new Server(serverBlocks[i]));
		}
		catch(std::exception& e)
		{
			Logger::error(e.what());
		}
	}
	if (serverList.empty())
		throw std::runtime_error("No server could bind.");
	return (serverList);
}

void	destroyServers(std::vector<Server*>& serverList)
{
	for (size_t i = 0; i < serverList.size(); i++)
		delete serverList[i];
	serverList.clear();
}
