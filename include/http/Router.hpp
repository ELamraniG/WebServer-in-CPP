#pragma once
#include "../config/Parser.hpp"
#include "HTTPRequest.hpp"


class Router {
public:
  static Server_block &match_server(const HTTPRequest &req,
                                    std::vector<Server_block> &servers);

  static location_block *match_location(const HTTPRequest &req,
                                        Server_block &server);
};
