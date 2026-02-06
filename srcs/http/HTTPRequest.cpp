#include "../../includes/http/HTTPRequest.hpp"
HTTPRequest::HTTPRequest()
    : method(""), uri(""), vers(""), body(""), querString(""),
      isComplete(false), isChunked(false), contentLength(0) {}

  const std::string &HTTPRequest::getMethod() const
  {
    return method;
  }
  const std::string &HTTPRequest::getURI() const
  {
    return uri;
  }
  const std::string &HTTPRequest::getVersion() const
  {
    return vers;
  }
  const std::string &HTTPRequest::getQueryString() const
  {
    return querString;
  }
  const std::string &HTTPRequest::getBody() const
  {
    return body;
  }
  std::string HTTPRequest::getHeader(const std::string &key) const
  {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
      return it->second;
    }
    return "";
  }
  bool HTTPRequest::isItCompleted() const
  {
    return isComplete;
  }
  bool HTTPRequest::isItChunked() const
  {
    return isChunked;
  }
  size_t HTTPRequest::getContentLength() const
  {
    return contentLength;
  }

  void HTTPRequest::setMethod(const std::string &method)
  {
    this->method = method;
  }
  void HTTPRequest::setURI(const std::string &uri)
  {
    this->uri = uri;
  }
  void HTTPRequest::setVersion(const std::string &version)
  {
    this->vers = version;
  }
  void HTTPRequest::setQueryString(const std::string &query)
  {
    this->querString = query;
  }
  void HTTPRequest::setBody(const std::string &body)
  {
    this->body = body;
  }
  void HTTPRequest::setHeader(const std::string &key, const std::string &value)
  {
    this->headers[key] = value;
  }
  void HTTPRequest::setCompleted(bool complete)
  {
    isComplete = complete;
  }
  void HTTPRequest::setChunked(bool chunked)
  {
    isChunked = chunked;
  }
  void HTTPRequest::setContentLength(size_t length)
  {
      contentLength = length;
  }
  void HTTPRequest::clearEverything()
  {
      method.clear();
      uri.clear();
      vers.clear();
      headers.clear();
      body.clear();
      querString.clear();
      isComplete = false;
      isChunked = false;
      contentLength = 0;
  }