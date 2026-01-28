
#include "../../includes/config/Parser.hpp"
#include <exception>
#include <stdexcept>

Parser::Parser(std::vector<std::string>& tokens) : _tokens(tokens), state(GLOBAL), current_server(NULL), current_location(NULL) {}

Parser::~Parser() {}

std::vector<server_block> Parser::getServers() const {
    return servers;
}

void Parser::parse() {
   for(size_t i = 0; i < _tokens.size(); i++)
   {
        std::string token = _tokens[i];
        if(token == "server")
        {
            if(state != GLOBAL)
                throw std::runtime_error("server block found inside another block");
            if(i + 1 >= _tokens.size() || _tokens[i + 1] != "{")
                throw std::runtime_error("server must be followed by '{'");
            state = SERVER;
            server_block new_server;
            servers.push_back(new_server);
            current_server = &(servers.back());
            continue;
        }
        if(token == "location")
        {
            if(state != SERVER)
                throw std::runtime_error("location block needs to be inside server block");
            if(i + 2 >= _tokens.size() || _tokens[i + 2] != "{")
                throw std::runtime_error("location must be followed by '{'");
            location_block new_location;
            new_location.path = _tokens[i + 1];
            current_server->locations.push_back(new_location);
            state = LOCATION;
            i += 2;
            continue;
        }
   }
}
