#pragma once

#include "location_block.hpp"
#include <iostream>
#include <map>
class server_block {

    public:
        std::string port;
        std::string host;
        std::string root;
        std::string index;
        std::map<int, std::string> error_pages;
        unsigned long client_max_body_size;
        std::vector<location_block> locations;
        server_block();
        ~server_block();
};