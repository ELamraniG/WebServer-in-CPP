#pragma once
#include "../config/Parser.hpp"
#include "Request.hpp"

class Router{
    static location_block *match_server(Request& req, std::vector<server_block> servers);
    static location_block *match_location(Request& req, server_block server);
    static std::string get_path(server_block server);

};