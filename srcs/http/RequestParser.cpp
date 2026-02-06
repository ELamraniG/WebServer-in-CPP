#include "../../includes/http/RequestParser.hpp"

RequestParser::RequestParser() {}

RequestParser::ParsingStatus
RequestParser::parseRequest(const std::string &rawBytes, HTTPRequest &request) {
  return P_INCOMPLETE;
}

bool RequestParser::parseOneRequestLine(const std::string &one_line,
                                        HTTPRequest &request) {
  return false;
}

bool RequestParser::parseHeaders(const std::string &theHeader,
                                 HTTPRequest &request) {
  return false;
}

bool RequestParser::parseBody(const std::string &theBody,
                              HTTPRequest &request) {
  return false;
}

void RequestParser::resetEverything() {}
