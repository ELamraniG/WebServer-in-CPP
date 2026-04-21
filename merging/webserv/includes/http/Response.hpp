#pragma once

#include <map>
#include <string>

// simple struct to hold what the method handlers return
// Person 3 can move this or extend it later for the response builder

struct Response {
  int statusCode;
  std::string contentType;
  std::string body;
  std::map<std::string, std::string> headers;

  Response() : statusCode(200), contentType("text/html") {}
};
