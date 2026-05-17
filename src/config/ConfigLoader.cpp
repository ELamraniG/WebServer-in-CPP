#include "../../include/config/ConfigLoader.hpp"
#include "../../include/config/Tokenizer.hpp"
#include "../../include/config/Parser.hpp"

#include <stdexcept>

std::vector<Server_block>	parseConfig(const char* configFile)
{
	std::vector<Server_block>	serverBlocks;
	Tokenizer					tokenizer(configFile);
	Parser						parser(tokenizer.tokens);

	parser.parse();
	serverBlocks = parser.getServers();
	if (serverBlocks.empty())
		throw std::runtime_error("No server blocks found in config.");
	return (serverBlocks);
}
