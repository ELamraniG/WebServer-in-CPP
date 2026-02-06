#pragma once

#include <map>
#include <string>

#include "HTTPRequest.hpp"

class RouteConfig;

struct CGIResult {
  bool success;
  int statusCode;
  std::map<std::string, std::string> headers;
  std::string body;
  std::string errorMessage;
};

class CGIHandler {
public:
  CGIHandler();

  bool isCGIRequest(const std::string &uri, const RouteConfig &route);
  CGIResult execute(const HTTPRequest &request, const RouteConfig &route);
  char **buildEnvironment(const HTTPRequest &request);

private:
  std::string buildScriptPath(const std::string &uri,
                              const RouteConfig &route) const;
  void freeEnvironment(char **envp) const;
};

