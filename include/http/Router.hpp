#pragma once
#include "../config/location_block.hpp"
#include "../config/Server_block.hpp"
#include "HTTPRequest.hpp"


class Router {
public:
  static const location_block *match_location(const HTTPRequest &req,
                                        const Server_block &server);
};
