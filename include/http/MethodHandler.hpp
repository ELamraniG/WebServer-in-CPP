#pragma once

#include <string>

#include "HTTPRequest.hpp"
#include "Response.hpp"
#include "RouteConfig.hpp"

class MethodHandler {
public:
  MethodHandler();
  static Response makeError(int code, const std::string &msg,const RouteConfig &route);
  Response handleGET(HTTPRequest &request, const RouteConfig &route);
  Response handlePOST(HTTPRequest &request, const RouteConfig &route);
  Response handleDELETE(HTTPRequest &request, const RouteConfig &route);
  std::string getTheFileType(const std::string &path) const;
};
