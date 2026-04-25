#pragma once
#include "../config/Parser.hpp"
#include "HTTPRequest.hpp"

// Router: matches incoming HTTPRequest to the appropriate Server_block and
// location_block
class Router {
public:
  // Match the best Server_block for a request (by host header and port)
  // Returns first server if no match found
  static Server_block &match_server(const HTTPRequest &req,
                                    std::vector<Server_block> &servers);

  static Server_block &match_server(const HTTPRequest & /*req*/,
                                   std::vector<Server_block> &servers,
                                   const std::string &local_host,
                                   const std::string &local_port);

  // Match the best location_block within a server (longest-prefix match)
  // Returns NULL if no location matches
  static location_block *match_location(const HTTPRequest &req,
                                        Server_block &server);
};
