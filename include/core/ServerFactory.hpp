#pragma once

class Server;
class Server_block;

#include <vector>

std::vector<Server*>	createServers(std::vector<Server_block>& serverBlocks);
void					destroyServers(std::vector<Server*>& serverList);