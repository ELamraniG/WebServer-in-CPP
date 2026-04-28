#pragma once 

#include "Server_block.hpp"
#include "location_block.hpp"
#include <string>
#include <vector>
#include <set>
#include <sstream>

typedef enum{
    GLOBAL,
    SERVER,
    LOCATION
} block_type;

class Parser {
    private:
        std::vector<std::string>    _tokens;
        std::vector<Server_block>   servers;
        block_type                  state;
        Server_block*               current_server;
        location_block*             current_location;
        std::set<std::string>       _server_seen;
        std::set<std::string>       _location_seen;

        void parse_server_block(size_t &i);
        void parse_location_block(size_t &i);
    public:
        Parser(std::vector<std::string> &tokens);
        ~Parser();
        void parse();
        std::vector<Server_block> getServers() const;
};

bool          isvalidport(std::string& port);
int           isvalid_error_number(std::string& error_number);
unsigned long isvalid_client_number(std::string& client_number);
void          print_servers(const std::vector<Server_block>& servers);
int           isvalid_return_number(std::string& return_number);
