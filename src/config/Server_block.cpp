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
    // FIX: port defaults to "" (not "80"). The parser uses emptiness as the
    // "was listen directive ever set?" signal and rejects server blocks
    // that forgot to list one.
    port = "";
    // FIX: 0.0.0.0 binds all interfaces. The previous default "127.0.0.1"
    // silently made your server reachable only from localhost, which causes
    // failures when the evaluator tests from another host or uses siege
    // against the machine's real IP.
    host = "0.0.0.0";
    root = "";
    index = "";
    // 1 MiB default. Parser overrides when client_max_body_size is set.
    client_max_body_size = 1048576;
}

Server_block::~Server_block() {}
