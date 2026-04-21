#include "../../includes/config/server_block.hpp" 


location_block::location_block() {
    path = "";
    root = "";
    index = "";
    autoindex = false;
    upload_pass = "";
}

location_block::~location_block() {
}

server_block::server_block() {
    port = "80";        
    host = "127.0.0.1";
    root = "";
    index = "";
    client_max_body_size = 1000000;
}

server_block::~server_block() {
}