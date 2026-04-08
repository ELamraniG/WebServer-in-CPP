#pragma once

#include "ChunksDecoding.hpp"
#include "HTTPRequest.hpp"
#include <string>

class RequestParser {
public:
  enum ParsingStatus { P_SUCCESS, P_INCOMPLETE, P_ERROR };

  RequestParser();

  ParsingStatus parseRequest(const std::string &rawBytes, HTTPRequest &request);
  bool parseFirstLine(const std::string &one_line, HTTPRequest &request);
  bool parseHeaders(const std::string &theHeader, HTTPRequest &request);

private:
  ChunksDecoding chunksDecoding;
};
