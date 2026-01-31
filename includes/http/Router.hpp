#pragma once
#include "../config/Parser.hpp"
#include "Request.hpp"

class Router{
    public:
        static server_block& match_server(Request& req, std::vector<server_block>& servers);
        static location_block *match_location(Request& req, server_block& server);
        static std::string get_path(Request& req, server_block& server, location_block* location);

};