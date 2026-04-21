#include "../../includes/http/RequestParser.hpp"
#include <cstdlib>
#include <sstream>
/*
  RAW HTTP REQUEST FORMAT:

  Complete example of what arrives on the socket (as raw bytes):

  GET /search?q=hello&lang=en HTTP/1.1\r\n
  Host: example.com\r\n
  Content-Length: 13\r\n
  Content-Type: application/json\r\n
  \r\n
  {"data":"ok"}

  PARSING BREAKDOWN:

  1. REQUEST LINE (split by spaces, ends with \r\n)
     GET /search?q=hello&lang=en HTTP/1.1\r\n
     ├─ METHOD: "GET"
     ├─ URI: "/search?q=hello&lang=en"
     │  ├─ Path (before ?): "/search"
     │  └─ Query String (after ?): "q=hello&lang=en"
     └─ VERSION: "HTTP/1.1"

  2. HEADERS (each line: KEY: VALUE\r\n, until \r\n\r\n)
     Host: example.com\r\n
     Content-Length: 13\r\n
     Content-Type: application/json\r\n
     ├─ host: "example.com"
     ├─ content-length: "13"
     └─ content-type: "application/json"

  3. BLANK LINE (\r\n\r\n marks end of headers)

  4. BODY (13 bytes based on Content-Length)
     {"data":"ok"}

  OFFSETS IN RAW BYTES:
  - Position 0-44: "GET /search?q=hello&lang=en HTTP/1.1\r\n"
  - Position 44-64: Headers
  - Position ~87-89: "\r\n\r\n"
  - Position ~91+: Body starts
*/
RequestParser::RequestParser() {}

static std::string strTrim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

static std::string strToLower(const std::string &s) {
  std::string out = s;
  for (size_t i = 0; i < out.size(); ++i)
    out[i] = std::tolower((out[i]));
  return out;
}

RequestParser::ParsingStatus
RequestParser::parseRequest(const std::string &rawBytes, HTTPRequest &request) {

  // look for the /r/n line end
  size_t firstLineEnd = rawBytes.find("\r\n");
  if (firstLineEnd == std::string::npos)
    return P_INCOMPLETE;

  // get the requestline
  std::string requestLine = rawBytes.substr(0, firstLineEnd);
  if (!parseFirstLine(requestLine, request))
    return P_ERROR;

  // look for /r/n/r/n end of the headers
  size_t headersEnd = rawBytes.find("\r\n\r\n");
  if (headersEnd == std::string::npos)
    return P_INCOMPLETE;

  // headers are always between /r/n which is at the end of the first line, and
  // /r/n/r/n end of headers
  std::string theHeader =
      rawBytes.substr(firstLineEnd + 2, headersEnd - firstLineEnd - 2);
  if (!parseHeaders(theHeader, request))
    return P_ERROR;

  // 5. get the body start pos after the /r/n/r/n of the end of the headers
  size_t bodyStart = headersEnd + 4; // right after \r\n\r\n

  // check if the body is chunked or has length
  std::string transferEncoding = request.getHeader("transfer-encoding");
  if (strToLower(transferEncoding) == "chunked") {
    request.setChunked(true);

    if (bodyStart >= rawBytes.size())
      return P_INCOMPLETE;
    std::string chunkedData = rawBytes.substr(bodyStart);
    std::string decodedBody;
    DecodeResult res = chunksDecoding.decode(chunkedData, decodedBody);
    if (res == DECODE_NEED_MORE)
      return P_INCOMPLETE;
    if (res == DECODE_ERROR)
      return P_ERROR;
    request.setBody(decodedBody);
    request.setCompleted(true);
    return P_SUCCESS;
  }

  std::string contentLength = request.getHeader("content-length");
  if (!contentLength.empty()) {
    char *tmp = NULL;
    unsigned long contentLengthValue =
        std::strtoul(contentLength.c_str(), &tmp, 10);
    if (tmp == contentLength.c_str())
      return P_ERROR; // not a valid number

    request.setContentLength(contentLengthValue);
    if (rawBytes.size() - bodyStart < contentLengthValue)
      return P_INCOMPLETE;

    request.setBody(rawBytes.substr(bodyStart, contentLengthValue));
    request.setCompleted(true);
    return P_SUCCESS;
  }

  // no body no problem
  request.setContentLength(0);
  request.setCompleted(true);
  return P_SUCCESS;
}

bool RequestParser::parseFirstLine(const std::string &one_line,
                                   HTTPRequest &request) {
  std::istringstream iss(one_line);
  std::string method, uri, version;
  if (!(iss >> method >> uri >> version))
    return false;
  // if there is more than get | index | http
  std::string extra;
  if (iss >> extra)
    return false;
  // invalide method
  if (method != "GET" && method != "POST" && method != "DELETE")
    return false;

  // invalide hhtp v
  if (version != "HTTP/1.0")
    return false;

  // parse the query string : ?lopo=lapa
  // ?lopo=lapa&kapa=kopa&yes=no
  size_t stifhamPos = uri.find('?');
  if (stifhamPos != std::string::npos) {
    request.setQueryString(uri.substr(stifhamPos + 1));
    uri = uri.substr(0, stifhamPos);
  }

  request.setMethod(method);
  request.setURI(uri);
  request.setVersion(version);
  return true;
}

// each line of header if key:value /r/n at the end also couldbe no header
bool RequestParser::parseHeaders(const std::string &theHeader,
                                 HTTPRequest &request) {
  if (theHeader.empty())
    return true;
  size_t pos = 0;
  while (pos < theHeader.size()) {
    size_t lineEnd = theHeader.find("\r\n", pos);
    std::string line;
    if (lineEnd == std::string::npos) {
      line = theHeader.substr(pos);
      pos = theHeader.size();
    } else {
      line = theHeader.substr(pos, lineEnd - pos);
      pos = lineEnd + 2;
    }
    if (line.empty())
      continue;
    // split with the first: key : value
    size_t posOfDots = line.find(':');
    if (posOfDots == std::string::npos)
      return false;
    
    std::string key = strTrim(line.substr(0, posOfDots));
    std::string value = strTrim(line.substr(posOfDots + 1));

    if (key.empty())
      return false;
    request.setHeader(strToLower(key), value);
  }
  return true;
}
