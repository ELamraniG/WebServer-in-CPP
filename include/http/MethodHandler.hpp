#pragma once

#include <string>

#include "HTTPRequest.hpp"
#include "Response.hpp"
#include "RouteConfig.hpp"

class MethodHandler {
public:
  MethodHandler();
  static Response makeError(int code, const std::string &msg,const RouteConfig &route);
  Response handleGET(const HTTPRequest &request, const RouteConfig &route);
  Response handlePOST(const HTTPRequest &request, const RouteConfig &route);
  Response handleDELETE(const HTTPRequest &request, const RouteConfig &route);
  std::string getTheFileType(const std::string &path) const;
};
