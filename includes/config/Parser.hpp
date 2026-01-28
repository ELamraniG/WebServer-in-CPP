#pragma once 

#include "server_block.hpp"
#include "location_block.hpp"
#include <string>
#include <vector>

typedef enum{
    GLOBAL,
    SERVER,
    LOCATION
} block_type;

class Parser {
    private:
        std::vector<std::string> _tokens;
        std::vector<server_block>   servers;
        block_type state;
        server_block*   current_server;
        location_block* current_location;
        void parse_server_block();
        void parse_location_block();
    public:
        Parser(std::vector<std::string> &tokens);
        ~Parser();
        void parse();
        std::vector<server_block> getServers() const;
     
};