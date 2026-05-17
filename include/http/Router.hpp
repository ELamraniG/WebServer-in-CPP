#pragma once
#include "../config/Parser.hpp"
#include "HTTPRequest.hpp"


class Router {
public:
  static const location_block *match_location(const HTTPRequest &req,
                                        const Server_block &server);
};
