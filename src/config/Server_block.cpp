#include "../../include/config/Server_block.hpp"

location_block::location_block() {
    path = "";
    root = "";
    index = "";
    autoindex = false;
    upload_pass = "";
}

location_block::~location_block() {}

Server_block::Server_block() {

    port = "";
    host = "0.0.0.0";
    root = "";
    index = "";
    client_max_body_size = 1048576;
}

Server_block::~Server_block() {}
