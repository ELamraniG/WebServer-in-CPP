#include "../../include/http/HTTPRequest.hpp"

static std::string trimSpaces(const std::string &s) {
  size_t start = s.find_first_not_of(" \t");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

static void
parseCookiesFromHeader(const std::string &value,
                       std::map<std::string, std::string> &cookies) {
  size_t pos = 0;

  cookies.clear();
  while (pos < value.size()) {
    size_t sep = value.find(';', pos);
    std::string part;
    if (sep == std::string::npos)
      part = value.substr(pos);
    else
      part = value.substr(pos, sep - pos);
    size_t eq = part.find('=');
    if (eq != std::string::npos) {
      std::string key = trimSpaces(part.substr(0, eq));
      std::string val = trimSpaces(part.substr(eq + 1));
      if (!key.empty())
        cookies[key] = val;
    }
    if (sep == std::string::npos)
      break;
    pos = sep + 1;
  }
}

HTTPRequest::HTTPRequest()
    : method(""), uri(""), vers(""), body(""), queryString(""),
      isComplete(false), isChunked(false), contentLength(0) {}

const std::string &HTTPRequest::getMethod() const { return method; }
const std::string &HTTPRequest::getURI() const { return uri; }
const std::string &HTTPRequest::getVersion() const { return vers; }
const std::string &HTTPRequest::getQueryString() const { return queryString; }
const std::string &HTTPRequest::getBody() const { return body; }
std::string HTTPRequest::getHeader(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it = headers.find(key);
  if (it != headers.end()) {
    return it->second;
  }
  return "";
}
const std::map<std::string, std::string> HTTPRequest::getAllHeaders() const
{
	return (headers);
}
std::string HTTPRequest::getCookie(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it = cookies.find(key);
  if (it != cookies.end()) {
    return it->second;
  }
  return "";
}
bool HTTPRequest::isItCompleted() const { return isComplete; }
bool HTTPRequest::isItChunked() const { return isChunked; }
size_t HTTPRequest::getContentLength() const { return contentLength; }

void HTTPRequest::setMethod(const std::string &method) {
  this->method = method;
}
void HTTPRequest::setURI(const std::string &uri) { this->uri = uri; }
void HTTPRequest::setVersion(const std::string &version) {
  this->vers = version;
}
void HTTPRequest::setQueryString(const std::string &query) {
  this->queryString = query;
}
void HTTPRequest::setBody(const std::string &body) { this->body = body; }
void HTTPRequest::setHeader(const std::string &key, const std::string &value) {
  this->headers[key] = value;
  if (key == "cookie")
    parseCookiesFromHeader(value, this->cookies);
}
void HTTPRequest::setCompleted(bool complete) { isComplete = complete; }
void HTTPRequest::setChunked(bool chunked) { isChunked = chunked; }
void HTTPRequest::setContentLength(size_t length) { contentLength = length; }