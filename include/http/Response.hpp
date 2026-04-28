#pragma once

#include <map>
#include <string>


struct Response {
  int statusCode;
  std::string contentType;
  std::string body;
  std::map<std::string, std::string> headers;

  Response() : statusCode(200), contentType("text/html") {}
};
