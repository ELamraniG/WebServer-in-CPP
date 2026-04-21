#pragma once
#include "../config/Parser.hpp"
#include "HTTPRequest.hpp"

// Router: matches incoming HTTPRequest to the appropriate server_block and
// location_block
class Router {
public:
  // Match the best server_block for a request (by host header and port)
  // Returns first server if no match found
  static server_block &match_server(const HTTPRequest &req,
                                    std::vector<server_block> &servers);

  // Match the best location_block within a server (longest-prefix match)
  // Returns NULL if no location matches
  static location_block *match_location(const HTTPRequest &req,
                                        server_block &server);
};
